# Contrato Formal del Proyecto

## 1. Partes

* **Solicitantes:** Alejandro Rodríguez Costello, Walter Lozano
* **Responsable del desarrollo:** Juan Ponce

Se acuerda el presente contrato para la definición y ejecución del proyecto descrito en este documento.

## 2. Objeto del contrato

Desarrollar un worker ejecutable por CLI en C para integración con Google Sheets API, con capacidad de:

* Resolver el `spreadsheetId` a partir de una URL completa de Google Sheets o de un ID directo.
* Consultar metadata de una spreadsheet pública mediante API Key (sin OAuth).
* Validar la respuesta contra un schema fijo definido por el proyecto.
* Persistir el payload obtenido en una base SQLite.
* Inferir y versionar schemas del payload ante cambios estructurales del proveedor.
* Podar campos no reconocidos o finalizar la ejecución de forma controlada si el cambio de schema impide continuar.

## 3. Alcance funcional

El proyecto incluirá:

1. Recepción por línea de comandos de una URL de Google Sheets o de un `spreadsheetId` directo, con extracción del ID en ambos casos.
2. Conexión TCP construida a mano (sockets propios) + TLS mediante OpenSSL contra `sheets.googleapis.com`, puerto 443.
3. Armado manual del request HTTP/1.1 (GET) contra el endpoint `/v4/spreadsheets/{spreadsheetId}`, usando la API Key como query param.
4. Validación de la respuesta recibida contra un schema fijo definido por el proyecto (campos mínimos esperados: `spreadsheetId`, `properties.title`, `sheets[].properties`).
5. Persistencia del payload JSON validado (sin los campos desconocidos) en una base SQLite local (tabla `assets`).
6. Cálculo de un identificador de schema (hash de la estructura de campos) para detectar cambios estructurales entre ejecuciones.
7. Versionado del schema cuando se detecta una diferencia: snapshot del estado anterior guardado en `assets_history` antes de sobreescribir.
8. Manejo ante cambios de schema incompatibles: podar los campos no reconocidos, o finalizar la ejecución con mensaje de error claro, sin crashear.
9. Manejo de errores HTTP (400, 401, 403, 404, 429, 5xx) sin interrupción abrupta del programa.

## 4. Entregables

* Código fuente funcional del worker, escrito en C.
* Script o instrucciones de compilación y ejecución por línea de comandos.
* Definición de modelo de datos mínimo: `assets`, `assets_history`.
* Documento de Contrato (este documento).
* Documento de Workplan del proyecto.
* README.md técnico del repositorio (dependencias, requisitos previos, compilación/uso, comandos CLI, flujo de la app, modelo de tests, estructura del proyecto, documentación a  generar).
* Documentación técnica en `Documentacion/`: análisis de la base de datos, configuración de la API, implementación y evidencias.

## 5. Criterios de aceptación

Se considerará cumplido el contrato cuando:

1. El worker resuelva correctamente el `spreadsheetId` a partir de una URL o de un ID directo.
2. Se establezca una conexión TCP + TLS exitosa contra `sheets.googleapis.com` usando sockets construidos manualmente.
3. Se obtenga una respuesta válida de la API usando únicamente la API Key (sin OAuth).
4. La respuesta sea validada contra el schema fijo definido, y se evidencie el comportamiento ante un cambio de schema (poda de campos o finalización controlada).
5. Se evidencie el almacenamiento del payload capturado en la base SQLite (`assets`).
6. El sistema detecte correctamente cambio y no cambio de schema, versionando en `assets_history` cuando corresponda.
7. Se mantenga trazabilidad del dato: a partir de la base podemos conocer qué recursro se guardó, cuándo fue su última captura y cuándo cambió su estructura.
8. Los errores HTTP (400/401/403/404/429/5xx) sean manejados sin interrupción abrupta del programa.

## 6. Exclusiones

No se incluye en este proyecto:

* Lectura de valores de celdas (contenido real de la hoja) — el alcance es exclusivamente metadata (`includeGridData=false`).
* Escritura o modificación de la spreadsheet (no soportado con API Key simple; requeriría OAuth 2.0 o Service Account).
* Autenticación OAuth 2.0 / Service Account.
* Edición de formato (colores, fuentes, bordes, fusión de celdas), fórmulas, gráficos o macros.
* Creación o borrado de spreadsheets o de pestañas.
* Cola de tareas con control de concurrencia (no aplica al alcance de este proyecto).
* Desarrollo de interfaz gráfica (frontend).
* Despliegue productivo en infraestructura final.
* Soporte posterior a la entrega fuera del período académico.