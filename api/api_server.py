# api/api_server.py

from flask import Flask, request, send_file, render_template_string
from PIL import Image
import io

app = Flask(__name__)

# --- CONFIGURACIÓN ---
# La e-paper es 250x122. La imagen ocupa la mitad izquierda (125x122)
IMG_WIDTH = 125
IMG_HEIGHT = 122
# La pantalla usa 1 bit por píxel (1bpp)
TARGET_BPP = 1 
OUTPUT_SIZE_BYTES = (IMG_WIDTH * IMG_HEIGHT) // 8 # 1907 bytes

# HTML simple para el formulario de carga
HTML_FORM = """
<!doctype html>
<title>Subir Imagen a E-Paper</title>
<h1>Subir Imagen</h1>
<form method=post enctype=multipart/form-data>
  <input type=file name=file>
  <input type=submit value=Subir>
</form>
"""

@app.route('/', methods=['GET', 'POST'])
def upload_file():
    if request.method == 'POST':
        if 'file' not in request.files:
            return 'No hay parte de archivo', 400
        file = request.files['file']
        if file.filename == '':
            return 'No se seleccionó archivo', 400
        if file:
            try:
                # 1. Abrir la imagen subida
                img = Image.open(file.stream)
                
                # 2. Preprocesamiento: Escalar y Recortar
                # Escalar la imagen para ajustarse a 125x122 manteniendo la proporción
                img.thumbnail((IMG_WIDTH, IMG_HEIGHT))
                
                # Crear una imagen negra (fondo) para centrar la imagen escalada
                new_img = Image.new('L', (IMG_WIDTH, IMG_HEIGHT), 255) # L (8-bit grayscale), 255=White
                
                # Calcular la posición para centrar
                offset = ((IMG_WIDTH - img.width) // 2, (IMG_HEIGHT - img.height) // 2)
                new_img.paste(img, offset)

                # 3. Conversión a 1-bit Monocromo
                # Usamos el modo 1 de PIL (1bpp). 0=Negro, 255=Blanco. 
                # La e-paper a menudo requiere invertir esto, donde 0x00 es negro y 0xFF es blanco.
                final_img = new_img.convert('1', dither=Image.FLOYDSTEINBERG)
                
                # 4. Convertir la imagen 1bpp de PIL al formato de bytes crudos para el e-paper
                # PIL almacena los bits por columna/fila dependiendo del driver;
                # Aquí asumimos el formato estándar que el driver C++ esperará.
                
                # Guardar el buffer de imagen binario en un objeto en memoria
                binary_buffer = io.BytesIO()
                final_img.save(binary_buffer, format='PPM', bits=1)
                
                # NOTA CLAVE: El formato de bytes de PIL puede NO coincidir con el
                # que necesita el e-paper. Un e-paper necesita 1907 bytes de un
                # formato de barrido específico (fila a fila o columna a columna).
                
                # Aquí simulamos la respuesta de bytes crudos.
                # En un entorno real, usarías lógica como esta para generar el buffer:
                byte_data = []
                for row in range(IMG_HEIGHT):
                    for col_byte in range(IMG_WIDTH // 8):
                        byte_val = 0
                        for bit_index in range(8):
                            x = col_byte * 8 + bit_index
                            pixel = final_img.getpixel((x, row))
                            # Los pixeles en modo '1' son 0 (Negro) o 255 (Blanco)
                            if pixel < 128: # Si es negro
                                byte_val |= (1 << (7 - bit_index)) # Setear el bit
                        byte_data.append(byte_val)
                
                # 5. Devolver el buffer de bytes crudos
                # Esta es la respuesta que el ESP8266 recuperará
                response_data = bytes(byte_data)
                
                if len(response_data) != OUTPUT_SIZE_BYTES:
                    # Debería ser 1907.
                    print(f"ERROR DE TAMAÑO: {len(response_data)} bytes generados, esperado {OUTPUT_SIZE_BYTES}")
                    
                
                # Retornar el buffer binario
                return send_file(
                    io.BytesIO(response_data),
                    mimetype='application/octet-stream',
                    as_attachment=True,
                    download_name='image_buffer.bin'
                )

            except Exception as e:
                return f'Error al procesar la imagen: {e}', 500

    return render_template_string(HTML_FORM)

if __name__ == '__main__':
    # Usar un puerto diferente al 80, ya que el ESP8266 lo usará.
    # Esta API se correría en tu computadora local.
    # El ESP8266 accedería a esta API para obtener la imagen.
    app.run(host='0.0.0.0', port=5000)