# IoT Threat Monitor

**IoT Threat Monitor** es un proyecto de carácter experimental enfocado en el diseño, implementación y validación de un prototipo automatizado para la detección de amenazas en entornos domóticos. Se basa en el análisis pasivo del tráfico de red y el monitoreo en tiempo real de dispositivos IoT, con el objetivo de identificar vulnerabilidades, configuraciones inseguras y comportamientos anómalos.

## Configuración básica de Mosquitto y Telegraf

Para establecer autenticación entre el microcontrolador (ESP32) y Mosquitto, así como entre Telegraf y el broker MQTT, es necesario definir usuarios y contraseñas. Algunas credenciales vienen configuradas por defecto, pero se recomienda modificarlas para mejorar la seguridad.

### Crear usuario para el ESP32

```bash
mosquitto_passwd -c ./mosquitto/config/password_file [name_client]
```

### Crear usuario para Telegraf

Asegúrate de actualizar también el archivo `telegraf.conf` con las nuevas credenciales:

```bash
mosquitto_passwd ./mosquitto/config/password_file [name_client]
```

### Configuración en InfluxDB

Las credenciales de acceso a InfluxDB también deben ser modificadas para mayor seguridad. Estas se encuentran definidas en el archivo `docker-compose.yml`, bajo la sección correspondiente a InfluxDB.

## Permisos para ejecutar Grafana

Para evitar errores de permisos al iniciar Grafana (ya que corre bajo el UID 472 por defecto), se debe asignar correctamente la propiedad del volumen de datos local:

```bash
sudo chown -R 472:472 ./grafana
```

Esto garantiza que Grafana pueda leer y escribir en su volumen persistente sin problemas.

## Arquitectura

```bash
                    ┌────────────────────────┐
                    │       Laptop / PC      │
                    │  - Grafana             │
                    │  - SSH al Raspberry Pi │
                    └─────────▲──────────────┘
                              │
                        (LAN / WiFi)
                              │
     ┌────────────┐     ┌─────┴─────┐     ┌────────────────────┐
     │ Cámara IoT │<--->│  Router   │<--->│ Raspberry Pi       │
     │ (WiFi)     │     │  WiFi/LAN │     │ - Docker           │
     └────────────┘     └───────────┘     │ - Mosquitto        │
                                          │ - Grafana          │
                                          │ - Influxdb         │
                                          │ - Telegraf         │
                                          └─────────▲──────────┘
                                                   │
                                                 QMTT
                                                   │
                                     ┌─────────────┴──────────────┐
                                     │     TTGO T-Beam ESP32      │
                                     │ - Sniffer modo promiscuo   │
                                     └────────────────────────────┘
```
