# Especificación del Protocolo de Chat CI-4835

## A) Visión General del Protocolo

El Protocolo de Chat CI-4835 es un protocolo de capa de aplicación basado en texto, cliente-servidor y orientado a conexión  TCP.

### Reglas de Sintaxis

1. **Codificación:** Todo el texto intercambiado DEBE estar codificado en UTF-8.
2. **Delimitación de Líneas:** Cada comando del cliente y respuesta del servidor DEBE finalizar estrictamente con la secuencia CRLF (`\r\n`).
3. **Longitud Máxima:** Las líneas enviadas por el cliente NO DEBEN exceder los 512 bytes, incluyendo la secuencia CRLF final. Las respuestas del servidor pueden exceder este límite.
4. **Insensibilidad a Mayúsculas/Minúsculas:** Los nombres de los comandos NO distinguen entre mayúsculas y minúsculas
5. **Separación de Argumentos:** Los argumentos de los comandos se separan por un único espacio (`0x20`). El último argumento (generalmente texto) PUEDE contener espacios.
6. **Formato de Respuestas Multilínea:** Sigue la convención de SMTP (RFC 5321). 

---

## B) Gramática RFC 5234 del Protocolo

La especificación formal de la sintaxis utiliza la Notación RFC 5234.

### Reglas Léxicas del cliente

| tipo | Descripcion |
| :--- | :--- |
| nickname | 1*16( ALPHA / NUM / "-" ) |
| channel | "#" 1*32( ALPHA (sin acentos) / NUM / "-" ) |
| text | *( UTF8-CHAR ), Texto sin CRLF |
| msg-id | 1*NUM, Identificador numérico de mensaje |


### Sintaxis de los Comandos del Cliente 


| comando | modo de uso|
| :--- | :--- |
| nick | "NICK" nickname |
| join | "JOIN" channel |
| part | "PART" channel |
| msg | "MSG" channel text |
| priv | "PRIV" nickname text |
| chans | "CHANS" |
| who | "WHO" channel |
| stat | "STAT" |
| list | "LIST" |
| retr | "RETR" msg-id |
| dele | "DELE" msg-id |
| quit | "QUIT" |


---

## C) Clases de Código

Los códigos de estado están categorizados numéricamente según la clase de respuesta:
* **2xx:** Éxito (el comando solicitado se procesó correctamente).
* **3xx:** Eventos Asíncronos (notificaciones enviadas por el servidor sin previa solicitud del cliente).
* **4xx:** Errores Temporales (fallos condicionales; el comando podría funcionar en intentos posteriores).
* **5xx:** Errores Permanentes (sintaxis inválida o violaciones de protocolo no reintentables).

### Reglas Lexicas del Servidor

| tipo | Descripcion |
| :--- | :--- |
|code | 3DIGIT |
|single-line | code text |
|multi-line | *( code-text ) code text |


### Tabla de Codigos

| Código | Formato de Respuesta (2xx, 3xx) / Significado (4xx, 5xx) | Descripción |
| :--- | :--- | :--- |
| **200** | nickname listo | Saludo inicial enviado inmediatamente al establecer la conexión TCP. |
| **201** | text | Registro exitoso de nickname mediante NICK. |
| **210** | text | Suscripción exitosa a un canal mediante JOIN. |
| **211** | text | Salida exitosa de un canal mediante PART |
| **212** | multi-line:<br>212-channel <n> <br>212 <total> canales | Lista de canales existentes y cantidad de miembros por canal |
| **213** | multi-line:<br>213-nickname<br>213 <n> miembros | Lista de miembros presentes en el canal especificado. |
| **220** | <n> <bytes> | Resumen del buzón: número de mensajes <n> y tamaño total en bytes <bytes> |
| **221** | multi-line:<br>221-<número> <bytes><br>221 <n> mensajes | Listado abreviado del buzón mostrando el ID del mensaje y su tamaño |
| **222** | multi-line:<br>222-<línea_hdr_o_cuerpo><br>222 fin del mensaje | Contenido completo del mensaje con cabeceras RFC 5322 |
| **223** | text | Marcado exitoso de un mensaje para su eliminación |
| **250** | text | Mensaje de canal (MSG) o mensaje privado en vivo (PRIV) entregado con éxito |
| **251** | text | Mensaje privado almacenado en el buzón en disco (destinatario desconectado) |
| **299** | text | Confirmación de cierre de sesión (QUIT). Servidor procederá a cerrar el socket. |
| **300** | channel nickname: text | Notificación asíncrona: mensaje publicado en un canal del que el cliente es miembro. |
| **301** | nickname text | Notificación asíncrona: mensaje privado recibido |
| **302** | channel nickname <evento> | Notificación asíncrona: un usuario entra, sale o se desconecta del canal. |
| **303** | <n> | Notificación asíncrona enviada tras un NICK exitoso si el usuario tiene <n> mensajes pendientes en buzón. |
| **421** | Servidor saturado |  |
| **433** | Apodo en uso |  |
| **442** | No eres miembro del canal |  |
| **500** | Comando desconocido |  |
| **501** | Sintaxis inválida |  |
| **502** | Línea demasiado larga |  |
| **530** | No registrado |  |
| **550** | Usuario desconocido |  |
| **551** | Canal inexistente |  |
| **552** | Mensaje inexistente |  |

---
## D) Extensiones propias

dentro de la carpeta src el archivo de cabecera commos.h coniene definiciones en comun para los .cpp

---
## E) Casos borde

existen dos casos a tratar:
* **PRIV a uno mismo**: el cliente SI se puede hacer PRIV a si mismo (inspirado en whatsapp y telegram)
* **Persitencia de canales** los canales no persisten, y cuando un cliente hace quit, se sale de los canales
* **Interrupcion de escritura del cliente** si el servidor hace una notificacion asincrona mientras el cliente escribe, esta se sobrepone en su escritura, pero no la interrumpe, es decir, puede el cliente estar escibiendo un comando sin percatarse que fue cortado por el mensaje del servidor, que igual va a duncionar el comando del cliente si era valido 
