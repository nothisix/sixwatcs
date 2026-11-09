import os
import paramiko
import time

HOST = "192.168.1.27"
USER = "bry"
PASS = "kali"
PORT = 22
REMOTE_DIR = "/home/bry/sixwatch-webflash"

def deploy():
    print(f"[Deploy] Connecting to {USER}@{HOST}:{PORT}...")
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(HOST, port=PORT, username=USER, password=PASS, timeout=15)
    print("[Deploy] SSH connected successfully!")

    # 1. Create remote directory structure
    stdin, stdout, stderr = ssh.exec_command(f"mkdir -p {REMOTE_DIR}/public/assets")
    stdout.channel.recv_exit_status()

    # 2. Upload files via SFTP
    sftp = ssh.open_sftp()
    local_base = "webflash"

    upload_list = [
        ("server.js", f"{REMOTE_DIR}/server.js"),
        ("public/index.html", f"{REMOTE_DIR}/public/index.html"),
        ("public/manifest.json", f"{REMOTE_DIR}/public/manifest.json"),
        ("public/firmware.bin", f"{REMOTE_DIR}/public/firmware.bin"),
        ("public/bootloader.bin", f"{REMOTE_DIR}/public/bootloader.bin"),
        ("public/partitions.bin", f"{REMOTE_DIR}/public/partitions.bin"),
        ("public/boot_app0.bin", f"{REMOTE_DIR}/public/boot_app0.bin"),
        ("public/assets/sixwatch_banner.png", f"{REMOTE_DIR}/public/assets/sixwatch_banner.png"),
        ("public/assets/sixwatch_logo.png", f"{REMOTE_DIR}/public/assets/sixwatch_logo.png"),
        ("public/assets/wiring_diagram.png", f"{REMOTE_DIR}/public/assets/wiring_diagram.png")
    ]

    for local_rel, remote_path in upload_list:
        local_path = os.path.join(local_base, local_rel)
        if not os.path.exists(local_path):
            print(f"[Error] Local file not found: {local_path}")
            continue
        size = os.path.getsize(local_path)
        print(f"[SFTP] Uploading {local_rel} ({size:,} bytes) -> {remote_path}...")
        sftp.put(local_path, remote_path)

    sftp.close()
    print("[Deploy] All files uploaded via SFTP!")

    # 3. Start or restart service on PM2
    start_cmd = (
        f"cd {REMOTE_DIR} && "
        "pm2 delete sixwatch-webflash 2>/dev/null || true; "
        "pm2 start server.js --name sixwatch-webflash && "
        "pm2 save"
    )
    print(f"[Deploy] Running PM2 startup command...")
    stdin, stdout, stderr = ssh.exec_command(start_cmd)
    exit_status = stdout.channel.recv_exit_status()
    print(stdout.read().decode())
    err = stderr.read().decode()
    if err:
        print("[PM2 Stderr]:", err)

    time.sleep(2)

    # 4. Verification test from localhost
    test_cmd = "curl -Is http://127.0.0.1:8500/ | head -n 5; echo '--- MANIFEST ---'; curl -s http://127.0.0.1:8500/manifest.json | grep -o '\"name\": *\"[^\"]*\"'"
    stdin, stdout, stderr = ssh.exec_command(test_cmd)
    print("[Verification Test Result]:")
    print(stdout.read().decode())

    ssh.close()
    print("[Deploy] Deployment completed successfully!")

if __name__ == '__main__':
    deploy()
