# Demo-Projekt zum Vortrag Oxidieren Schritt für Schritt

## Überblick

Dies ist ein WiFi-Scanner als Demo-Projekt zum Vortrag [Oxidieren Schritt für
Schritt](https://github.com/sirhcel/ese-congress-2025). Ist für das [T-Display
S3](https://lilygo.cc/products/t-display-s3?variant=42284559827125) von Lilygo
erstellt und zeigt die Integration von Rust in eine (nicht ganz)
Bare-Metal-Firmware. Sämtliche Punkte einer Integration von Rust in eine
Bare-Metal-Mikrocontroller-Firmware können hieran jedoch nachvollzogen werden.


## Ausgangspunkt: WLAN-Scanner

Ausgang für eigene Versuche ist der im Hauptzweig zu findende WLAN-Scanner.
Dieser such nach Netzwerken und zeigt eine Auswahl der gefundenen Netzwerke auf
dem Display an.


## Ziel: Anzeige von QR-Codes zum Anmelden am jeweiligen Netzwerk

Die im Vortrag gezeigten Punkte zur Integration von Rust in eine C-Anwendung
lassen sich am Beispiel des WLAN-Scanners nachvollziehen. Ziel ist das Erzeugen
von QR-Codes mit Hilfe der Rust-Crate [qrcode](https://crates.io/crates/qrcode)
und die Ausgabe auf dem Display mit Hilfe des vom Scanner genutzten
[LVGL](https://docs.lvgl.io/8.3/).


## Bau und Flashen

* Einrichten der Entwicklungsumgebung für das ESP-IDF
    * [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use esp-idf to build projects.
    * Das Beispiel nutzt Version 5.3.0
* Einrichten der Rust-Toolchain
    * [The Rust on ESP Book - Toolchain Installation](https://docs.espressif.com/projects/rust/book/getting-started/toolchain.html)
    * Das Lilygo T-Display S3 hat Xtensa-Kerne - dem [passenden Abschnitt](https://docs.espressif.com/projects/rust/book/getting-started/toolchain.html#xtensa-devices) folgen
* Projekt konfigurieren und übersetzen
    * Repository klonen
        ```
        $ git clone https://github.com/sirhcel/ese-congress-2025-demo.git
        ```
    * Projekt konfigurieren und dabei die Option 7, _T-Display-S3_, auswählen
        ```
        ese-congress-2025-demo$ python3 ./setup.py
        ```
    * Das Projekt wird gebaut
    * Projekt flashen
        ```
        ese-congress-2025-demo$ idf.py flash monitor
        ```
    * Der Monitor des ESP-IDF kann über `Ctrl` + `T`, `X` wieder verlassen werden

## Bei Fragen

* Dem [Matrix-Chat](https://matrix.to/#/#sirhcel-esec-2025-demo:matrix.org) beitreten
* [Diskussion in diesem Repository starten](https://github.com/sirhcel/ese-congress-2025-demo/discussions/new?category=general)


## Musterlösung

* Meine Musterlösung gibt es im Zweig [rusty-wifi-scanner](https://github.com/sirhcel/ese-congress-2025-demo/tree/rusty-wifi-scanner)
