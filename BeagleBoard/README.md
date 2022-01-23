# Komunikacja Ethernet na BeagleBone Black

Proponowany sposób komunikacji z płytką to połączenie SSH przy wykorzystaniu Windows Subsystem for Linux, które zawiera narzędzia takie jak: SSH klient, SCP, ifconfig - przydatne w eksperymentowaniu z tego typu zagadnieniami.

Pierwszym krokiem będzie przesłanie przygotowanego wcześniej skryptu Python na płytkę.

W tym celu należy najpierw przekopiować przygotowany skrypt do systemu plików w WSL.


W eksploratorze plików Windows jako ścieżkę należy wpisać:

```
\\wsl$
```
w ten sposób uzyskamy wygodny dostęp do plików w Linux. Należy przekopiować skrypt do folderu domowego (`/home/<user>`).

Należy podpiać płytkę do komputera poprzez USB, wystawi ona wirtualny interfejs sieciowy i będzie dla nas pod adresem:
 ```
192.168.7.2
 ```

Następnie wykorzystamy narzędzie `scp` do przesłania pliku, w następujący sposób:
```
scp PATH/send_packet.py debian@192.168.7.2:/home/debian
``` 
Program zapyta nas o hasło, które powinno być ustalone na domyślne, tzn.  `temppwd`.

Po przesłaniu pliku możemy z terminala WSL wygodnie dostać się do terminala płytki:
```
ssh debian@192.168.7.2
```
również podając wcześniej wspomniane hasło.

Większość wykonywanych poleceń będzie od nas wymagać uprawnień administratora, dlatego możemy przejść na konto root w następujący sposób:
```
sudo su
```
lub poprzedzać kolejne polecanie słowem `sudo`.

W celu uruchomienia skryptu i przesłania ramki poprzez interfejs sieciowy należy w konsoli wpisać:
```
python send_packet.py
```
Program wyślę ramkę i poinformuje nas o jej długości.  W celu edycji skryptu, np. zmiany payloadu, najwygodniej będzie skorzystać z edytorów konsolowych wbudowanych na płytce, np:
```
vim send_packet.py
```
Wychodzimy z niego poprzez kolejne naciśniecie klawiszy `esc`, `:`, `q`, `enter` ;)  

Do odbierania i obserwowania komunikacji na tego typu płytce z systemem Linux warto wykorzystać program `tcpdump`.

Nie jest on zainstalowany domyślnie, można to zrobić po podpięciu płytki do sieci internet i wpisaniu poleceń:
```
apt-get update
apt-get install tcpdump
```
Przykładowe wywołanie tego programu przedstawiono poniżej:

``` 
tcpdump -vv -i eth0 ether proto 0x9999
```
Flaga `-vv` służy do dokładniejszego wypisania informacji o odebranej ramce, wraz z odczytaniem payloadu. Flaga  i argument `-i eth0` informują na którym interfejscie będziemy obserwować komunikację. Natomiast `ether proto 0x9999` to dodatkowy filr, który pozwoli na obserwowanie tylko interesujących nas ramek powiązanych ze skryptem. Jeśli chcemy obserwować cały ruch sieciowy na danym interfejsie, bez szczegółów każdej odebranej ramki wystarczy polecenie:

``` 
tcpdump -i eth0
```

