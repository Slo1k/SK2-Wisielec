# SK2-Wisielec

### Sposób kompilacji
Dla server.cpp: g++ --std=c++2a -o server server.cpp

### Sposób uruchomienia:
- Dla server.cpp: ./server
- Dla primitive_client.py: python3 primitive_client.py

Domyślnie serwer oraz klient działają na localhost na porcie 54321

###  Dostępnę komendy dla klienta:
- SETNICK [TwojNick]                - Ustawia nick dla danego gracza, nie może on być pusty (NICKEMPTY) oraz nie może się powtarzać z aktualnie zajętymi (NICKTAKEN)
- ROOMS                             - Wyświela listę dostępnych pokoi, jeśli nie ma utworzonego żadnego pokoju serwer wysyła wiadomość (NOROOMS)
- JOIN [NazwaPokoju]                - Pozwala dołączyć do pokoju o podanej nazwie, jeśli pokój nie istnieję to tworzy pokój o podanej nazwię
- START                             - Pozwala na rozpoczęcie rozgrywki przez gracza który pierwszy dołączył do pokoju, następnie serwer wysyła do pozostałych graczy                                       prośbę o potwierdzenie gotowości. Jeśli grę spróbuje rozpocząć gracz nie posiadający uprawnienie to otrzyma wiadomość (NOKING)
- ACCEPT                            - Potwierdza gotowość gracza do rozpoczęcia rozgrywki
- GUESS [LITERA]                    - Pozwala podać literę do zgadywanego hasła
- LEAVE                             - Pozwala na opuszczenie pokoju, w przypadku bycia poza pokojem serwer wysyła wiadomość (OUTSIDEROOM)


### Autorzy:
Mikołaj Krakowiak 148076
Adam Kopiec 148198
