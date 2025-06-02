# 🧠 Tarea 3 - Principios de Sistemas Operativos

Repositorio para el control de versiones de la **Tarea 3** del curso **Principios de Sistemas Operativos**.

**Autor:** René Sánchez Torres y Jose Andrés Vargas Serrano

**Carné:** 2020051805 y 2019211290

**Institución:** Tecnológico de Costa Rica

---

## 🛠️ Descripción
En la actualidad, los servidores web enfrentan importantes retos relacionados con la escalabilidad y la administración eficiente de sus recursos. A pesar de que muchos de estos servidores están subutilizados durante la mayor parte del tiempo, existen momentos de alta demanda como la venta de entradas para eventos masivos donde estos sistemas colapsan al no poder gestionar múltiples conexiones simultáneamente.

La tarea propuesta busca simular este escenario mediante la construcción de un WebServer que utilice el protocolo HTTP 1.1 y sea capaz de responder a solicitudes utilizando: pre-threaded. Estos modelos permiten manejar múltiples conexiones mediante la creación anticipada de hilos. Además, se desarrollará un cliente HTTP capaz de interactuar con este servidor, y una herramienta de stress testing con el fin de saturarlo y evaluar su comportamiento bajo carga extrema, simulando un ataque.

---

## 🚀 Compilación

*WEB SERVER*
```bash
gcc prethreaded.c -o prethread-webserver -lpthread
```
*HTTP CLIENT*
```bash
gcc httpclient.c -o httpclient -lcurl
```
## 🦾 EJECUCIÓN 
WEB SERVER

*-n 100: Define el número de hilos pre cargados*
```bash
./prethread-webserver -n 60 -w ./www -p 8080
```

*METODOS DE HTTP*

**GET**
```bash
curl -v http://localhost:8080/index.html
```
```bash
curl -v http://localhost:8080/api/data.json
```
```bash
curl -v http://localhost:8080/directorio_sin_index
```

**HEAD**
```bash
curl -I http://localhost:8080/index.html
```
**POST**
```bash
curl -X POST -H "Content-Length: 23" -d "Contenido inicial del archivo" http://localhost:8080/miarchivo.txt -v
```

**PUT**
```bash
echo "Contenido de prueba" > prueba.txt
```
```bash
curl PUT prueba.txt http://localhost:8080/prueba.txt -v
```

**DELETE**
```bash
curl -X DELETE http://localhost:8080/imagen.jpg -v
```

*HTTP CLIENT*
```bash
./httpclient -u http://localhost:8080/imagen.jpg -o imagenDescargada.jpg
```
```bash
./httpclient -u http://localhost:8080
```

*STRESS CLIENT*

*-n 100: Define el número de hilos*

*./httpclient: Ruta al ejecutable del cliente en C*

*El resto son parámetros para el cliente*
```bash
python3 stressclient.py -n 10 -u http://localhost:8080```

