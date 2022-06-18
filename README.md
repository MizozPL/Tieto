# Tieto - CPU Ussage Tracker
## Kompilacja:  
W głównym folderze repozytorium należy wykonać:
```
cmake --configure .
cmake --build . --target Tieto
```
Jeżeli chcemy również uruchamiać testy:
```
cmake --build . --target WatchdogTest
cmake --build . --target QueueTest
```
---
## Uruchomienie:  
W głównym folderze repozytorium należy wywołać:
```
./src/Tieto
```
Analogicznie dla testów:
```
./test/WatchdogTest
./test/QueueTest
```
---
## Zamknięcie:  
Aplikację zamykamy wysyłając sygnał SIGTERM:  
```
kill -15 <pid>
```
