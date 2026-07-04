# Servicios IoT

El sistema incorpora tecnologías IoT para el monitoreo remoto y la notificación de eventos en tiempo real.

## ThingSpeak

ThingSpeak se utiliza para almacenar y visualizar datos generados por el sistema de seguridad.

Variables registradas:

- Distancia detectada por el sensor HC-SR04.
- Estado del sensor PIR.
- Estado de la alarma.
- Estado general del sistema.

## Gamil

Se utiliza gmail para enviar notificaciones automáticas al usuario cuando se detectan eventos relevantes.

Entre los mensajes enviados se incluyen:

- Sistema iniciado.
- Presencia detectada en la puerta.
- Intrusión confirmada.
- Desactivación del sistema.
