# Netcode Library


## Introduksjon

Netcode Library er en lettvekts C++-bibliotek for implementering av klient-server nettverkskommunikasjon i spill og sanntidsapplikasjoner. Dette prosjektet er utviklet som en del av et frivillig prosjekt i et nettverksprogrammeringskurs, med fokus på å lage nettkode for multiplayer-spill med prediksjonsalgoritmer.

## Implementert funksjonalitet

- Klient-server arkitektur
- Spilltilstand-synkronisering
- Klientside-prediksjon
- Entitetsinterpolering
- Lag-kompensasjon
- Pålitelig UDP-protokoll
- Pakketapshåndtering
- Low-latency kommunikasjon
- Trådsikker implementasjon
- Kryssplattform-kompatibilitet (Windows, macOS, Linux)

## Fremtidig arbeid / Nåværende mangler og svakheter

### Mangler
- Fullt implementert peer-to-peer støtte
- Automatisk tilkobling og gjenoppretting av forbindelse
- Komprimering av nettverksdata
- Avansert anti-cheat mekanismer

### Svakheter
- Nettverksbåndbredde kan bli en flaskehals ved mange spillere
- Prediksjonsalgoritmer kan føre til synlige hopp ved store avvik
- Håndtering av spillere med høy latens kan forbedres
- Ytelsesoptimalisering ved høy belastning

## Eksterne avhengigheter

- **C++20 Standard Library**: Grunnleggende datastrukturer og algoritmer
- **CMake (3.10+)**: Byggsystem for kryssplattform-kompilering
- **SFML (2.5.1)**: Simple and Fast Multimedia Library, brukt for grafisk demonstrasjon av prediksjonsalgoritmen
- **Catch2 (3.0.1)**: Testbibliotek for enhetstesting
- **spdlog (1.9.2)**: Høyytelses loggingsbibliotek for debugging

## Installasjonsinstruksjoner

### Forutsetninger

- C++20-kompatibel kompilator (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.10 eller høyere
- Git

### Bygging

bash

## Klon repoet
git clone (link)
cd netcode

## Oppsett av byggsystem

mkdir build && cd build cmake


## Kompilering
make -j$(nproc)

### Installasjon

# Installer biblioteket (valgfritt)
sudo make install

### Bruksinstruksjoner

## Kjøre demonstrasjonsprogrammet
# Fra build-mappen
./demo/netcode_demo

## Server-eksempel

#include <netcode/server.h>

int main() {
// Opprett en server på port 8080
netcode::Server server(8080);

    // Registrer callback for nye klienter
    server.onClientConnect([](netcode::ClientID client) {
        std::cout << "Klient " << client << " koblet til.\n";
    });
    
    // Start serveren (blokkerende kall)
    server.start();
    
    return 0;
}

## Klient-eksempel

#include <netcode/client.h>

int main() {
// Opprett en klient
netcode::Client client;

    // Koble til serveren
    if (client.connect("localhost", 8080)) {
        std::cout << "Koblet til serveren.\n";
        
        // Hovedloop
        while (client.isConnected()) {
            client.update();
            // Spillogikk
        }
    } else {
        std::cerr << "Kunne ikke koble til serveren.\n";
    }
    
    return 0;
}

## Kjøre tester

# Fra build-mappen
./tests/netcode_tests

For å kjøre ytelsestester:

./tests/netcode_performance_tests

### API-dokumentasjon
Detaljert API-dokumentasjon er tilgjengelig på [GitHub Pages] (link) eller lokalt i `docs/` mappen etter å ha kjørt:

# Generer dokumentasjon (krever Doxygen)
cd build
make docs

### Kildeinformasjon

Følgende eksterne ressurser har vært brukt som referanse under utviklingen:
- Gabriel Gambetta, ["Fast-Paced Multiplayer"](https://www.gabrielgambetta.com/client-server-game-architecture.html): Konsepter rundt klientside-prediksjon
- Glenn Fiedler, ["Networking for Game Programmers"](https://gafferongames.com/): Nettkode-konsepter og implementasjonsdetaljer
- Valve Developer Community, ["Source Multiplayer Networking"](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking): Lag-kompensasjon og entitetsinterpolering