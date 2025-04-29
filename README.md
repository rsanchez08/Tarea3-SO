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
HTTP CLIENT
Prueba
```bash
./httpclient -h localhost:8080 -m GET -p /
```
```bash
./prethread-webserver -n 10 -w ./www -p 8080
```
STRESS CLIENT
```bash
python3 stressclient.py -n 100 ./httpclient -- -h localhost:8080 -m GET -p /
```


