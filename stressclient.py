import sys
import threading
import subprocess

def attack(client_cmd):
    subprocess.run(client_cmd, shell=True)

def main():
    if '-n' not in sys.argv:
        print("Usage: stress -n <threads> HTTPclient <args>")
        return
    
    n_idx = sys.argv.index('-n')
    threads = int(sys.argv[n_idx+1])
    client_args = ' '.join(sys.argv[n_idx+2:])
    
    for _ in range(threads):
        threading.Thread(target=attack, args=(f'python3 {client_args}',)).start()

if __name__ == '__main__':
    main()