# Demo-Projekt zum Vortrag Oxidieren Schritt für Schritt

## Überblick

Dies ist ein WiFi-Scanner als Demo-Projekt zum Vortrag [Oxidieren Schritt für
Schritt](https://github.com/sirhcel/ese-congress-2025). Ist für das [T-Display
S3](https://lilygo.cc/products/t-display-s3?variant=42284559827125) von Lilygo
erstellt und zeigt die Integration von Rust in eine (nicht ganz)
Bare-Metal-Firmware. Sämtliche Punkte einer Integration von Rust in eine
Bare-Metal-Mikrocontroller-Firmware können hieran jedoch nachvollzogen werden.

> [!NOTE]
> Ich habe erste Rückmeldungen erhalten, die eine detailliertere Anleitung
> gewünscht haben. Ich arbeite die Vorschläge dazu schrittweise ein. Stellen
> Sie jederzeit gern [Fragen](#fragen).


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


## Einrichten der Entwicklungsumgebung

* [Einrichten der Entwicklungsumgebung](doc/setup.de.md)


## Bau und Flashen

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
* Der WiFi-Scanner sollte nun auf dem T-Display S3 laufen und seine
  Debug-Ausgaben im Monitor sichtbar sein
* Der Monitor des ESP-IDF kann über `Ctrl` + `T`, `X` wieder verlassen werden

## Bei Fragen

* Dem [Matrix-Chat](https://matrix.to/#/#sirhcel-esec-2025-demo:matrix.org) beitreten
* [Diskussion in diesem Repository starten](https://github.com/sirhcel/ese-congress-2025-demo/discussions/new?category=general)


## Musterlösung

* Meine Musterlösung gibt es im Zweig [rusty-wifi-scanner](https://github.com/sirhcel/ese-congress-2025-demo/tree/rusty-wifi-scanner)
