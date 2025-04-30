# 🧠 Tarea 3 - Principios de Sistemas Operativos

Repositorio para el control de versiones de la **Tarea 3** del curso **Principios de Sistemas Operativos**.

**Autor:** René Sánchez Torres y Jose Andrés Vargas Serrano

**Carné:** 2020051805 y 2019211290

**Institución:** Tecnológico de Costa Rica

---

## 🛠️ Descripción


---

## 🚀 Compilación

WEB SERVER
```bash
gcc prethreaded.c -o prethread-webserver -lpthread
```
HTTP CLIENT
```bash
gcc httpclient.c -o httpclient
```
## 🦾 EJECUCIÓN 
WEB SERVER
```bash
./prethread-webserver -n 10 -w ./www -p 8080
```

METODOS DE HTTP 
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
curl -X POST -H "Content-Type: application/json" -d '{"id":2,"nombre":"Rene","puesto":"QA"}' http://localhost:8080/api/data.json -v
```

**PUT**
```bash
echo "Contenido de prueba" > prueba.txt
```
```bash
curl -X PUT -T prueba.txt http://localhost:8080/prueba.txt -v
```

**DELETE**
```bash
curl -X DELETE http://localhost:8080/imagen.jpg -v
```

HTTP CLIENT
```bash
./httpclient -u http://localhost:8080/imagen.jpg -o imagenDescargada.jpg
```
```bash
```
STRESS CLIENT
*-n 100: Define el número de hilos*
*./httpclient: Ruta al ejecutable del cliente en C*
*El resto son parámetros para el cliente*
```bash
python3 stressclient.py -n 100 ./httpclient -h localhost:8080 -m GET -p /
```

