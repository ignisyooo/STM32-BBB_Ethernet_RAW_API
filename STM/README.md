# Komunikacja Ethernet na STM32

W celu komunikacji Ethernet przy użyciu przygotowana została biblioteka ETH_API.

W oprogoramowaniu STM32Cube_MX należy wyklikać prostą konfigurację mikrokontrolera uruchamiając

Przykładowa konfiguracja została przedstawiona poniżej:
![Screenshots](konfiguracja.png)

Opis funckjonalności:

```
void ETH_Proccess(void)
```
Funckja główna stworzonego API, którą należy umieścić w pętli głównej

```
void ETH_Send_data(const eth_address dst_mac, const uint8_t* data, uint16_t size)
```
Funckja do wysłania ramki danych pod wskazany adres. Jako parametr (`data`) należy podać adres payloadu, natomiast jako (`dst_mac`) należy podać 6 bajtowy adres MAC odbiorcy

Do analizy odebranej ramki przygotowana została struktura (`Ethernet_T`), która zawiera wszystkie pola otrzymanej ramki

Tablica (`receive_bytes`) zawiera dane otrzymane z ramki