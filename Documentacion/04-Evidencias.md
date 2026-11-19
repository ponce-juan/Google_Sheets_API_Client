# Evidencias

Registro de las pruebas realizadas sobre el API Client de Google Sheets.
Cada bloque indica el comando ejecutado, la salida real obtenida y qué demuestra.

- **Autor:** Ponce Juan
## Entorno de prueba:

| Componente | Detalle |
|---|---|
| Sistema Operativo | KDE Neon User Edition|
| Kernel/Arquitectura | Linux 7.0.0-43-generic x86_64 |
| Compilador | 13.3.0 |
| Make | GNU Make 4.3 |
| OpenSSL | 3.0.13 |
| SQLite | 3.45.1 |
| Red | IPv4 con IP pública fija (restricción /16 en la API Key) |


## Cómo leer este documento

- Las salidas están pegadas tal como las devolvió el programa, sin editar.
- Los tests locales (`make test`) no necesitan internet. Los tests de red (`make test-red`) sí.
- Las evidencias de tareas todavía no hechas figuran como **Pendiente**.

## Resumen de estado

| Tarea | Qué se prueba | Estado | Fecha |
|---|---|---|---|
| T3 | Socket TCP: tests locales | Hecho | 19/11/2026 |
| T3 | Socket TCP: conexión real a Google | Hecho | 19/11/2026 |
| T4 | TLS contra servidor local y contra Google | Pendiente | |
| T5 | Pedido HTTP y armado de URL | Pendiente | |
| T6 | Parser JSON y validación de schema | Pendiente | |
| T7 | Persistencia en SQLite | Pendiente | |
| T8 | Hash de schema y versionado | Pendiente | |
| T9 | Manejo de errores HTTP (400, 401, 403, 404, 429, 5xx) | Pendiente | |
| T10 | Ejecución completa contra una planilla real | Pendiente | |

---

## T3 - Socket TCP

### T3.1 Tests locales (sin internet)

**Comando:**

```bash
make test
```

**Salida:**

```text
ok  - conecta y transmite datos en ambos sentidos
ok  - puerto cerrado: falla con mensaje (conexión TCP: Connection refused)
ok  - host inexistente: falla en el DNS (resolución DNS: Name or service not known)
ok  - rechaza IPv6: el cliente solo usa IPv4 (resolución DNS: Address family for hostname not supported)
ok  - argumentos nulos y descriptor inválido
ok  - timeout: la conexión se corta a tiempo

Todos los tests de red pasaron.
```

**Qué demuestra:**

| Test | Demuestra |
|---|---|
| conecta y transmite | El socket se conecta y envía/recibe bytes en ambos sentidos. |
| puerto cerrado | Si nadie escucha, devuelve error con mensaje claro (no se cuelga). |
| host inexistente | Un nombre que no resuelve falla en la etapa de DNS. |
| rechaza IPv6 | El cliente usa solo IPv4, que es lo que exige la restricción de IP de la API Key. |
| argumentos nulos | Con argumentos inválidos devuelve error y no se rompe. |
| timeout | Si el destino no responde, la conexión se corta a tiempo. |

### T3.2 Conexión real a Google

**Comando:**

```bash
make test-red
```

**Salida:**

```text
Conexión TCP establecida con sheets.googleapis.com:443 (IPv4 172.217.112.4)
```

**Qué demuestra:** que el socket construido a mano llega a `sheets.googleapis.com:443` por IPv4. La IP puede cambiar entre ejecuciones porque Google rota las direcciones del nombre.

---

## T4 - TLS con OpenSSL

**Pendiente.**

## T5 - Pedido HTTP y armado de URL

**Pendiente.**

## T6 - Parser JSON y validación de schema

**Pendiente.**

## T7 - Persistencia en SQLite

**Pendiente.**

## T8 - Hash de schema y versionado

**Pendiente.**

## T9 - Manejo de errores

**Pendiente.**

## T10 - Ejecución completa

**Pendiente.**