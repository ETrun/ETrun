# ETrun ET: Legacy Server (Docker Image)

Docker image for running an **ET: Legacy** server with the **ETrun** mod.

---

## 🐳 Usage

```bash
docker run -d \
  -v $(pwd)/etmain:/basepath/etmain \
  -v $(pwd)/homepath:/homepath \
  -p 27960:27960/udp \
  -e DEFAULT_MAP=oasis \
  etrun/server
```

---

## 🔧 Environment Variables

| Variable         | Default    | Description                            |
|------------------|------------|----------------------------------------|
| `DEFAULT_MAP`    | `oasis`    | Map loaded at server start             |

---

## 📁 Volumes

| Container Path         | Purpose                 |
|------------------------|--------------------------|
| `/basepath/etmain`     | Maps, server configs     |
| `/homepath`            | Writable game data       |

---

## 👤 User

Runs as unprivileged user `legacy`.

---

## 🧪 Entrypoint (`run.sh`)

```bash
#!/bin/bash
set -e

exec /basepath/etlded.x86_64 +exec server.cfg \
+set fs_game etrun \
+set fs_basepath /homepath \
+set fs_homepath /basepath \
+map "${DEFAULT_MAP:-oasis}"
```

---

## 📄 License

MIT

---

## 🔗 Links

- [ETrun GitHub](https://github.com/ETrun/ETrun)
- [ET: Legacy](https://www.etlegacy.com)
