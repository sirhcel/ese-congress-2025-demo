# Entwicklungsumgebung einrichten

## Überblick

* Der hier eingesetzte ESP32-S3 ist ein Mikrocontroller mit zwei
  Xtensa-LX7-Kernen
* Xtensa ist wie ARM oder RISC-V eine eigene Prozessorarchitektur
* Die Unterstützung für diese Architektur ist noch nicht vollständig im
  Standard-Rust-Compiler verfügbar, aber Espressif bietet einen Fork des
  Rust-Compilers mit Unterstützung für Xtensa an

## Rust-Toolchain für ESP32-S3

* Zuerst die Rust-Toolchain für den Entwicklungsrechner nach [The Rust on ESP
  Book - Rust
  Installation](https://docs.espressif.com/projects/rust/book/getting-started/toolchain.html#rust-installation)
  einrichten
    * Über <https://rustup.rs/>
    * Suchpfad für die neu installierten Werkzeuge entweder automatisch
      erweitern lassen oder dies nach den Angaben des Installationsprogramms
      selbst tun
* Der Mikrocontroller besitzt zweit Xtensa LX7 Kerne, die der Standard-Rust-Compiler noch nicht unterstützt
* Espressif bietet für diese einen Fork des Rust-Compilers an, der die
  Unterstützung für diese Architektur mitbringt
    * Installation nach [The Rust on ESP Book - Toolchain
      Installation](https://docs.espressif.com/projects/rust/book/getting-started/toolchain.html#xtensa-devices)
    * Zuerst das Installationsprogramm `espup` installieren
        ```bash
        $ cargo install --locked espup
        ```
    * Mit dessen Hilfe die Rust-Toolchain für ESP mit Xtensa-Architektur
      installieren
        ```bash
        $ espup install
        ```
* Die Rust-Toolchain für den ESP32-S3 ist nun bereit

## ESP-IDF

* Die C-Codebasis nutzt Espressifs
  [ESP-IDF](https://github.com/espressif/esp-idf) in Version 5.3.4 auf
* Dessen [Getting Started
  Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)
  sieht für den ESP32-S3 die folgenden Schritte zur Installation und Einrichtung
  vor
    * Repository klonen
        ```bash
        $ git clone -b v5.3.4 --recursive https://github.com/espressif/esp-idf.git
        ```
    * Installation der Toolchain und Einrichtung für ESP32-S3
        ```bash
        esp-idf$ ./install.sh esp32s3
        ```
    * Am Ende der Installation angegeben, wie die Umgebung für das ESP-IDF
      eingerichtet werden soll, zum Beispiel
        ```bash
        esp-idf$ source ./export.sh
        ```
    * Es ist wichtig, diese Schritte nach der Einrichtung der Rust-Toolchain
      durchzuführen, da das ESP-IDF die von ihm installierte Version des GCC
      nutzen möchte
* Damit ist nun alles bereit, um die C-Firmware des Beispiels zu bauen
* Weiter geht es in [Bau und Flashen](../README.md#bau-und-flashen)
