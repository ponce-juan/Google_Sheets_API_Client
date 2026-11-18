# Configuración de la API de Google Sheets

Guía para dejar lista la API Key que usa el cliente (`sheets_client`). Complementa la sección *Autenticación* del [README](../README.md).

## Resumen

| Dato | Valor |
|---|---|
| API | Google Sheets API v4 |
| Autenticación | API Key simple (sin OAuth 2.0), decisión tomada en clase |
| Restricciones de la key | Solo Google Sheets API + IP en notación CIDR `/16` |
| Variable de entorno | `SHEETS_API_KEY` |
| Alcance | Lectura de metadata de spreadsheets públicas |

## Por qué API Key y no OAuth

- Para leer la metadata de una spreadsheet **pública** no hace falta saber quién consulta: alcanza con identificar el proyecto, y eso es lo que hace la API Key.
- OAuth 2.0 obligaría a firmar tokens (JWT con RSA) o a abrir un flujo con navegador. Es mucho trabajo y queda fuera del foco de la materia (capa de aplicación y sockets).
- Limitación aceptada: con API Key **no se puede escribir** en la spreadsheet. Está declarado en las exclusiones del Contrato.

## Paso a paso en Google Cloud Console

Sitio: <https://console.cloud.google.com/>

1. **Crear (o elegir) un proyecto.** Arriba a la izquierda, al lado del logo, abrí el selector de proyectos y creá uno nuevo, o usá uno que ya tengas.
2. **Habilitar la Google Sheets API.** Menú lateral: *APIs & Services → Library*. Buscá "Google Sheets API" y hacé clic en *Enable*. Sin este paso, la key no puede llamar al servicio aunque exista.
3. **Crear la API Key.** *APIs & Services → Credentials → Create credentials → API key*. Google muestra la key en un cuadro: copiala.
4. **Restringir la key a Sheets.** Entrá a la key para editarla. En *API restrictions* elegí *Restrict key* y marcá solo "Google Sheets API". Así, aunque alguien consiga la key, no puede usarla con otras APIs de Google.
5. **Restringir por IP.** En la misma pantalla, en *Application restrictions* elegí *IP addresses* y cargá el bloque en notación CIDR (ver la sección siguiente).
6. **Guardar.** Hacé clic en *Save*. Una key recién creada o modificada puede tardar unos minutos en funcionar.

## Restricción por IP con máscara /16

El docente indicó restringir la key por IP con una máscara grande (`/16`). Cómo calcular el bloque:

1. Buscá "cuál es mi ip" en Google para ver tu IP pública actual. Ejemplo: `181.230.45.12`.
2. Quedate con los primeros dos números, poné los últimos dos en `0` y agregá `/16` al final: `181.230.0.0/16`.
3. Ese bloque cubre 65.536 direcciones. Si el proveedor de internet te reasigna otra IP dentro de ese rango, no hay que tocar nada.

**Si la IP sale del bloque** (por ejemplo, cambió el proveedor o te conectás desde otra red), la API responde 403 y hay que actualizar la restricción en Cloud Console con el bloque nuevo.

**Nota para el cliente en C:** la restricción es sobre direcciones IPv4. Al resolver `sheets.googleapis.com` con `getaddrinfo`, conviene pedir IPv4 (`ai_family = AF_INET`). Si la conexión saliera por IPv6, Google vería otra dirección de origen y bloquearía el pedido. Se comprobó en la práctica: `curl` sin `-4` recibió 403 y con `-4` funcionó.

## Preparar la spreadsheet de prueba

1. Creá o abrí una spreadsheet en <https://sheets.google.com>.
2. *Compartir → Acceso general → Cualquier persona con el enlace → Lector*.
3. Copiá la URL. El ID es el tramo entre `/d/` y `/edit`:

```text
https://docs.google.com/spreadsheets/d/1AbCdEfGhIjKlMnOpQrStUvWxYz/edit
                                       └──────────── ID ────────────┘
```

Con API Key no hay identidad de usuario detrás del pedido, así que si la spreadsheet no es pública la API responde 403 aunque la key sea válida.

## Guardar la key

```bash
export SHEETS_API_KEY="tu-api-key"
```

- La key **nunca** va en el código ni en el repositorio.
- Si querés que quede entre sesiones, agregá esa línea a `~/.bashrc`.
- Si usás un archivo local para guardarla, agregalo a `.gitignore`.
- En las capturas de pantalla, tapá la key.

## Verificación rápida (sin el cliente)

Antes de programar, conviene confirmar que la key funciona. Reemplazá `<ID>` por el de tu spreadsheet:

```bash
curl -4 -s "https://sheets.googleapis.com/v4/spreadsheets/<ID>?key=$SHEETS_API_KEY&fields=spreadsheetId,properties.title"
```

El `-4` fuerza IPv4. Sin él, `curl` puede salir por IPv6 y la restricción de IP (que es sobre IPv4) bloquea el pedido.

Respuesta esperada (HTTP 200):

```json
{
  "spreadsheetId": "1AbCdEfGhIjKlMnOpQrStUvWxYz",
  "properties": { "title": "Nombre de tu spreadsheet" }
}
```

## Problemas frecuentes

| HTTP | Mensaje típico | Causa | Qué hacer |
|---|---|---|---|
| 400 | `API key not valid. Please pass a valid API key.` | Key mal copiada o variable vacía | Revisar `echo $SHEETS_API_KEY` |
| 403 | `Permission denied on the service.` (`reason: ACCESS_DENIED`) | La restricción de IP bloqueó el pedido: salió por IPv6, o la IPv4 pública cayó fuera del bloque | Probar con `curl -4`. Si sigue, comparar `curl -4 -s https://ifconfig.me` con el bloque cargado y actualizarlo |
| 403 | `Google Sheets API has not been used in project...` | La API no está habilitada | Habilitarla en *Library* |
| 403 | `The caller does not have permission` | La spreadsheet no es pública | Compartirla como "Cualquier persona con el enlace" |
| 404 | `Requested entity was not found.` | El ID no existe | Revisar el ID |
| 429 | `Quota exceeded...` | Demasiadas consultas por minuto | Esperar y reintentar |

El primer 403 de la tabla es el que se observó en este proyecto; el resto son los mensajes esperables de la API y no todos se reprodujeron.

Estos códigos son los que el cliente debe manejar sin interrumpirse (criterio de aceptación 8 del Contrato).

## Si la key se filtra

Entrá a *Credentials*, eliminá la key o regenerala, y actualizá `SHEETS_API_KEY`. Si se subió al repositorio por error, regenerarla es obligatorio, porque borrar el archivo no elimina la key del historial de git.

## Configuración aplicada en este proyecto

| Dato | Valor |
|---|---|
| Proyecto de Google Cloud | Client-API-Google-Sheets |
| Fecha de creación de la key | 18/11/26 |
| API habilitada | Google Sheets API |
| Restricción de API | Solo Google Sheets API |
| Restricción de aplicación | IP addresses(ejemplo): 181.230.0.0/16 |
| Spreadsheet de prueba | https://docs.google.com/spreadsheets/d/1KUzlsIPNddBoDurk0tckXVn990qVnD8ztav57HQh-Yw |