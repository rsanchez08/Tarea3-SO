import sys
import subprocess
import concurrent.futures

def run_client(url):
    try:
        result = subprocess.run(
            ["./httpclient", "-u", url],
            capture_output=True,
            text=True,
            timeout=20
        )
        return result.stdout.strip()
    except subprocess.TimeoutExpired:
        return "Timeout"
    except:
        return "Error"

def main():
    if "-n" not in sys.argv or "-u" not in sys.argv:
        print("Uso: stressclient.py -n <num> -u <url>")
        return
    
    n = int(sys.argv[sys.argv.index("-n") + 1])
    url = sys.argv[sys.argv.index("-u") + 1]
    
    print(f"Iniciando test de stress con {n} solicitudes a {url}")
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=n) as executor:
        futures = [executor.submit(run_client, url) for _ in range(n)]
        
        for i, future in enumerate(concurrent.futures.as_completed(futures)):
            print(f"[Solicitud {i+1}] {future.result()}")

if __name__ == "__main__":
    main()