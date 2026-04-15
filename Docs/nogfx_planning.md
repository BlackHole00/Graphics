# Pianificazione NoGfx Api

# Analisi dei requisiti

Implementare l'api NoGfx in una libreria utilizzabile dal linguaggio di programmazione C:
- La libreria sarà distribuita come set di file oggetto e header C (compatibile con C++).
- Sarà possibile compilare la libreria da sorgenti se necessario. 

L'overhead imposto da tale libreria deve essere il più basso possibile:
- Si cerca un overhead non superiore al 5% rispetto al codice Metal equivalente. In riferimento, MoltenVk,in un caso d'uso generico introduce un overhead di circa il 10%, ma deve emulare un'API molto più grande (Vulkan).
<!-- TODO: Find reference, non mi fido troppo di Chatty -->
- Verranno valutati sia i casi gpu-bound, che cpu-bound per la valutazione della performance

La libreria deve funzionare su hardware Apple dotato di processore Apple Silicon:
- Verrà utilizzata l'API di accelerazione grafica Metal 4, che, grazie alle nuove features introdotte rende la conversione realistica.
- Si targeta quindi macOs 26 Tahoe

## Struttura multi-layer

L'api inizialmente sarà implementata al di sopra di Metal 4, tuttavia è necessario progettare la libreria in modo che sia possibile espandere in un possibile futuro la funzionalità anche ad altre piattaforme con altre Api di accelerazione grafica (come Vulkan).  
Siccome alcune piattaforme supportano più Api (ad esempio Windows, con DirectX e Vulkan, o MacOs, con Metal 3 e Metal 4), sarebbe possibile dover decidere quale utilizzare a start-up time. Si decide quindi di adottare un sistema a con backend hot-swappabile al momento di start-up dell'applicazione.  

È desiderabile anche avere controlli extra sull'utilizzo della libreria, specialmente negli ambienti di sviluppo, prima dell'ambiente di production. Si adotterà quindi un sistema a layer, ispirato dal funzionamento di Vulkan:

```
|     Applicazione     | --v
                           NoGfx library call
|  Layer di debugging  | <--
|  Layer di traduzione | --v
                           Native Api call
|     Metal/VK/DX      | <--
```

Il funzionamento del layer di debugging è quello di analizzare gli input di ogni funzione, validando i valori passati.  
L'ultimo layer è sempre quello di traduzione, che converte la chiamata a NoGfx nell'equivalente nativo.

Questa soluzione porta vantaggi in termini di separazione tra codice di gestione degli errori e di traduzione, portando quindi ad una semplice organizzazione.  
È possibile utilizzare anche più di due layer, nel caso si vogliano implementare altre funzionalità di debugging (api tracing, etc...)

A lato implementativo, un layer si presenterà come una grade vtable.
```c
typedef struct GpuLayer {
  bool (*gpuMalloc)(size_t bytes, size_t align, MEMORY memory, GpuResult* result);
  bool (*gpuFree)(void* ptr);
  // ...
} GpuLayer;
```

## Multithreading
L'articolo originario non specifica nel dettaglio come la sincronizzazione dovrebbe avvenire. Sono stati quindi scelte le seguenti guidelines:
- La creazione e distruzione di risorse deve poter avvenire in modo asincrono (thread-safe). Ciò comprende anche l'allocazione e deallocazione dei buffer.
- L'encoding di un `GpuCommandBuffer` è thread-unsafe, deve essere sincrona o esternamente sincronizzata dall'utente.
- L'accodamento dei messaggi nella `GpuQueue` deve poter avvenire in modo asincrono (thread-safe).

## Espansione dell'API
L'Api NoGfx risulta quasi totalmente completa, però omette alcuni dettagli, nel particolare:
- Inizializzazione della libreria.
- Selezione del device e dettagli di creazione della `GpuQueue`.
- Presentazione a schermo.
- Gestione degli errori (che possono generare dal layer di validazione o dall'API nativa sottostante).
- Accesso ai primitivi dell'api nativa.

#### Inizializzazione della libreria e selezione del device
Al momento dell'inizializzazione l'utilizzatore della libreria dovrà stabilire le proprietà di inizializzazione di essa. Per far ciò utilizzerà la struttura `GpuInitDesc`, specificando api sottostante da utilizzare, richiedendo o meno la validazione e fornendo layer extra di terze parti.

Una volta inizializzato la libreria, l'utilizzatore dovrà quindi selezionare un dispositivo fisico da utilizzare. Nel caso di dispositivi Apple, ce ne sarà sempre solo uno: la scheda grafica integrata al processore. Nel futuro, se la libreria verrà portata a Vulkan, sarà possibile che l'utilizzatore avrà più schede grafiche disponibili (ad esempio, sia un'integrata, che quella dedicata).  
Per far ciò sono disponibili le funzioni `gpuEnumerateDevices` e `gpuSelectDevices`. Ogni dispositivo è identificato da un ID numerico.
Si nota che l'utilizzatore non può chiamare altre funzioni prima di aver selezionato un dispositivo.

Al momento della terminazione dell'applicazione l'utente potrà chiamare `gpuDeinit`.

Segue quindi la parte di Api rilevante:

```c
typedef enum GPU_BACKEND {
  GPU_NONE = 0,
  GPU_METAL_4,
  GPU_VULKAN,
  // ... 
};

typedef enum GPU_DEVICE_TYPE {
  GPU_INTEGRATED,
  GPU_DEDICATED,
};

typedef struct GpuInitDesc {
  GPU_BACKEND backend;
  bool validationEnabled;
  GpuLayer* extraLayers;
  size_t extraLayerCount;
} GpuInitDesc;

typedef size_t GpuDeviceId;

typedef struct GpuDeviceInfo {
  GpuDeviceId identifier;
  const char* name;
  const char* vendor;
  GPU_DEVICE_TYPE type;
  // TODO: device capabilities, limits, etc...
} GpuDeviceInfo;

void gpuInit(const GPUInitDesc* desc, GPU_RESULT* result);
void gpuDeinit();

size_t gpuEnumerateDevices(GpuDeviceInfo* devices, size_t devices_size, GPU_RESULT* result);
void gpuSelectDevice(GpuDeviceId deviceId, GPU_RESULT* result);
```

#### Creazione della Queue
TODO: Su Metal non servirebbe specificare nient'altro in teoria. Probabilmente si intende qualcosa di simile alle QueueFamilies di vulkan, ma non saprei

#### Presentazione
TODO: Guarda cosa fanno Vulkan, WebGpu e SDL3_GPU. Metal è proprio semplice.

#### Gestione degli errori
La maggior parte delle api moderne lavorano con sistemi di callback per informare l'utente degli errori o problemi avvenuti. Questo tuttavia rende difficile sapere esattamente dove un'errore è originato, la libreria quindi utilizzerà una gestione degli errori a livello di chiamata a funzione.

Ogni funzione accetterà come ultimo paramentro un valore puntatore a `GPU_RESULT`. Allora, se sono avvenuti problemi con l'esecuzione della chiamata a funzione, verrà ritornato l'errore in tale variabile.  
Possiamo considerare il layer di validazione come adeguato se riesce a riconoscere tutte le configurazioni di comandi invalide prima che queste ultime raggiungino l'api nativa. Se quindi viene ricevuto un messaggio d'errore da essa (attraverso il callback), allora tale messaggio è da considerarsi bug nel layer di validazione.  
Questa osservazione rende un sistema basato effettivamente su "errori come risultato" compatibile con sistemi basati su callback.

Si nota che se il puntatore al result risulta `null`, allora il valore risultato verrà semplicemente scartato.

#### Pseudo C++
Notiamo infine che l'api è in pseudo C++, quindi non utilizzabile da C o da linguaggi compatibili con esso. Vengono proposte alcune finali modifiche:
- Rimozione di valori di default
- Utilizzo di puntatore+lunghezza al posto di `std::span` e `ByteSpan`.
- Modifica dei tipi enum, in modo da avere un _namespace_ di appartenenza (`MEMORY_DEFAULT` -> `GPU_MEMORY_DEFAULT`).
Viene anche applicata l'ottimizzazione di passare grandi strutture attraverso puntatori const.

Segue quindi l'api da implementare nel completo:
```c
      \    /\
       )  ( ')
      (  /  )
todo   \(__)|
```
<!-- NOTA: Siccome il documento verrà stampato (e mettere codice nella tesi non piace in generale), considera se è meglio fornire un link a github. -->

## Analisi dell'API e risoluzione delle incrongruenze

### Buffers and memory menagement
Per buffer si intende un blob continuo di memoria acessibile dalla scheda grafica ed, opzionalmente, dalla cpu.

L'api ammette molteplici tipologie di buffer:
  - `MEMORY_DEFAULT`: il buffer risiede nella memoria della scheda grafica, ma è accessibile in scrittura dalla cpu attraverso _cpu mapped gpu pointers_.
  - `MEMORY_GPU`: il buffer risiede nella memoria della scheda grafica. Non è accessibile dalla gpu direttamente.
  - `MEMORY_READBACK`: il buffer risiede nella memoria della scheda grafica, ma è accessibile in lettura dalla cpu.

#### MEMORY_DEFAULT Buffers
L'api NoGfx richiede che alcune risorse residenti nella gpu siano accessibili dalla cpu attraverso semplici puntatori, grazie ad una tecnologia chiamata _cpu mapped gpu pointers_.  
Si nota subito che uno stesso buffer può essere identificato da due tipi di puntatori:
  - _cpu mapped virtual address_: puntatore accessibile dalla cpu. La scrittura su questo puntatore comporta la modifica della memoria della scheda grafica
  
  - _gpu virtual address_: puntatore accessibile solo dalla gpu. Deve tuttavia essere ottenibile a lato CPU per fare riferimento ai contenuti interni di un buffer.

Siccome i dispositivi Apple utilizzano un'architettura per la memoria unificata, la semantica del buffer di tipologia `MEMORY_DEFAULT` risulta molto simile ad un `MTLBuffer` generato con la resource option `MTLStorageModeShared | MTLResourceCPUCacheModeWriteCombined | MTLResourceHazardTrackingModeUntracked`. Dal punto di vista semantico della memoria non vi sono grandi incongruenze.

NoGfx prevede la seguente API:
  - `gpuMalloc(size, MEMORY_DEFAULT)`: Ritorna il _cpu mapped virtual address_ corrispondente con l'allocazione.Tale valore identificherà l'allocazione. 
  - `gpuHostToDevicePointer(ptr)`: Dato un _cpu mapped virtual address_ ritorna il corrispondente _gpu virtual address_.

Si nota come i due puntatori vengano trattati come oggetti differenti, invece di essere raggruppati gli uni con gli altri.

Metal utilizza invece un approccio differente. Ogni allocazione viene trattata come un "fat pointer", che permette di accedere in ogni momento sia al _cpu mapped virtual address_, che al _gpu virtual address_. Si analizzino infatti i seguenti metodi della classe `MTLBuffer`:
  - `- (void*) contents`: ritorna il _cpu mapped virtual address_ 
  - `- (MTLGPUAddress) gpuAddress`: ritorna il _gpu virtual address_ _(nota: `MTLGPUAddress` è equivalente a `size_t`)_

Sorge quindi un'incongruenza tra api. L'implementazione di `gpuMalloc` risulterebbe triviale, tuttavia non è possibile dire lo stesso di `gpuHostToDevicePointer`, siccome questa funzione deve poter accettare qualsiasi puntatore, anche quindi con un offset rispetto all'allocazione originaria.

<!-- Si è pensato quindi ad una soluzione in doppio passaggio: -->
  <!-- - 1: Lookup lineare per la risoluzione di puntatori senza offset all'allocazione problematica. Verrà usata una mappa del tipo `map[void*]MTLBuffer`. Se la mappa non contiene un associazione con il puntatore richiesto, allora sarà necessario effettuare il secondo passaggio -->
  <!-- - 2: Ricerca del puntatore con l'offset in uno storage di allocazioni. Si prevede di usare un albero binario basato sui bit del puntatore. L'albero avrà altezza $h <= 48$, in quanto, come amd64, anche in arm64 solo i 48 bit meno significativi vengono utilizzati per codificare il puntatore attuale (diversamente da amd64, i primi 16 bit vengono usati per contenere metadati, che possono essere ignorati). Ci si aspetta quindi una complessità $O(h) = O(48) = O(1)$. La soluzione a due passaggi è necessaria in quanto la ricerca nell'albero ha costante moltiplicativa molto più alta. (TODO: Accennare alla possibilità di analizzare la percentuale di chiamate a `gpuHostDeviceMemory` per determinare se il primo passaggio ha effettivamente senso). -->
  <!-- - 2: Ricerca del puntatore con l'offset in uno storage di allocazioni. Si prevede di usare un albero binario basato sui bit del puntatore. L'albero avrà altezza $h <= 48$, in quanto, come amd64, anche in arm64 solo i 48 bit meno significativi vengono utilizzati per codificare il puntatore attuale (diversamente da amd64, i primi 16 bit vengono usati per contenere metadati, che possono essere ignorati). Ci si aspetta quindi una complessità $O(h) = O(48) = O(1)$. La soluzione a due passaggi è necessaria in quanto la ricerca nell'albero ha costante moltiplicativa molto più alta. (TODO: Accennare alla possibilità di analizzare la percentuale di chiamate a `gpuHostDeviceMemory` per determinare se il primo passaggio ha effettivamente senso). -->

<!-- È possibile riutilizzare la mappa `map[void*]MTLBuffer` per l'implementazione della funzione `gpuFree`. -->

Si nota infine che, per come specificato nell'originario articolo, un buffer `MEMORY_DEFAULT` non dovrebbe permettere operazioni di lettura. Non vi è alcun modo di fare in modo che il _cpu mapped virtual address_ sia accessibile solo in scrittura. La lettura infatti funzionerebbe senza problemi (sebbene la tipologia di buffer `MTLResourceCPUCacheModeWriteCombined` comporti overhead).

#### MEMORY_GPU Buffers
Questi tipi di buffer sono identificati da un unico tipo di puntatori: solo da _gpu virtual pointer_. Segue quindi che la funzione `gpuMalloc(size, MEMORY_GPU)` non ritornerà un _cpu mapped virtual address_, bensì un _gpu virtual address_.

Il corrispettivo più adatto all'implementazione di questo tipo di buffer è un `MTLBuffer` creato con la resource option `MTLStorageModePrivate | MTLResourceTrackingModeUntracked`, accessibile quindi solo dalla scheda grafica.

#### MEMORY_READBACK Buffers
Questi tipi di buffer sono estremamente simili ai buffer di tipologia `MEMORY_DEFAULT`.
Il corrispettivo è un `MTLBuffer` con resource option `MTLStorageModeShared | MTLResourceCPUCacheModeDefaultCache | MTLResourceHazardTrackingModeUntracked`. L'opzione `MTLResourceCPUCacheModeDefaultCache` garantisce una tipologia di gestione della cache più lenta, ma ottimale per la lettura dal buffer.


### Textures

#### Allocazione
Le texture, nell'api NoGraphics, vengono create a partire da un _gpu address_, specificando quindi esplicitamente la locazione di memoria dove allocare la texture.  
Fortunatamente Metal fornisce il metodo `- (MTLTexture) newTextureWithDescriptor` nella classe `MTLBuffer`. L'implementazione della funzione `gpuCreateTexture` risulta quindi immediata, richiedendo solamente la traduzione da _gpu address_ a `MTLBuffer` ed offset in esso.

Notiamo inoltre che Metal è alquanto _restrittivo_ quando si parla di creazione di texture, attraverso `MTLBuffer`: diversi tipi di texture richiedono diverse grandezze ed alignment. Fortunatamente NoGraphics prevede la funzione `gpuTextureSizeAlign`, che, dato una specifica descrizione della texture, ritorna i requisiti in termini di spazio ed alignment richiesto.  
Per quanto riguarda l'implementazione di tale funzione è quindi necessario meticolosamente confrontare la descrizione fornita dall'utente con la funzionalità offerte dal `MTLDevice`, utilizzando quindi metodi come `minimumLinearTextureAlignmentForPixelFormat`.

Notiamo che nulla vieta che in un singolo buffer siano allocate più texture, anche allo stesso offset in esso. Fortunatamente Metal supporta questo tipo di allocazioni sovrapposte senza problemi.

#### Deallocazione
Per quanto riguarda la deallocazione, la situazione è più problematica: una texture non viene direttamente deallocata, ma l'api prevede la deallocazione del buffer contenitore, attraverso una semplice `gpuFree`.

Questo comportamento non si mappa bene con Metal, che utilizza una API object oriented con reference counting. Infatti se vi effettuasse l'operazione `release` al buffer contenitore, quest'ultimo non verrebbe veramente deallocato, in quanto esitono texture che fanno riferimento ad esso.  
Per ogni texture, notiamo inoltre, possono essere definite più _viste_ su di essa, le quali vanno _rilasciate_, insieme alla texture vera e propria per permettere la deallocazione del buffer.

Segue, che per ogni buffer è necessario ricordarsi le texture associate ad esso, e per ogni texture è necessario ricordarsi le _viste_ associata ad essa. La deallocazione richiede l'effettuazione dell'operazione _release_ su tutti quegli oggetti.

#### Texture Descriptors e Texture Heap
Ogni vista sulle texture è identificabile dalla scheda grafica (nelle shaders) attraverso un identificativo chiamato _texture descriptor_, di 32 byte. Quest'ultimo, in un mondo ideale, fornirebbe alla scheda grafica tutte le informazioni necessarie per identificare la memoria delle texture, la modalità di lettura dei texel ed il formato dell'informazione.  
Metal 4 tuttavia non espone direttamente i _texture descriptor_, optando invece per fornire all'utente solamente un `MTLResourceID` (8 byte), con il quale il driver ed il runtime MSL sono in grado di accedere al _texture descriptor_.  
Questo id, è purtroppo globale all'intera applicazione. Idealmente un _texture descriptor_ dovrebbe essere locale ad un heap di appartenenza. Ad ogni draw-call sarebbe necessario associare un solo _texture heap_. Questo dettaglio implementativo non è tuttavia riproducibile in Metal 4.

<!-- Questa differenza, unita ad un diverso modo di specifica delle risorse utilizzate per ogni operazione, permette a Metal di essere leggermente più flessibile. -->

Sebbene non sia totalmente ottimale, è possibile immettere all'interno dei _texture descriptor_ il `MTLResourceID`, sprecando quindi 24 byte, che devono rimanere di padding.  
Il _texture heap_ rimane un buffer di _texture descriptor_, che in realtà non sono veri descriptor nativi, ma solo puntatori ad essi gestiti dal runtime metal, il quali li trasformerà in veri descriptor.  
Passiamo da un ideale modello con singola indirezione `textureHeap[index]` ad un modello con doppia indirezione, una delle quali implicita: `managedRealHeap[uint64_t(userTextureHeap[index])]`.

Le funzioni `gpuTextureViewDescriptor` e `gpuTextureViewDescriptorRW`, ritornanti il _texture descriptor_, risultano quindi immediate.

<!-- In _NoGraphics_, la funzione `gpuSetActiveTextureHeap` accetta un array (accessibile dal cpu-side) di _texture descriptor_, al fine di preparare le texture all'utilizzo. Possiamo sfruttare questa funzione per ottimizzare la libreria, specificando, attraverso un `MTLResidencySet`, le texture che indentiamo usare, minimizzando l'overhead del driver. È necessario tuttavia conoscere la `MTLTexture` corrispondente al _texture descriptor_ fornito. -->  
<!-- La soluzione trovata è quella di utilizzare 8 dei molti byte liberi del _texture descriptor_, facendo riferimento all'oggetto `MTLTexture` corrispondente. -->

Il _texture heap_ (realmente un semplice buffer) deve poter essere manipolato anche a lato gpu (attraverso shader compute). Questa funzionalità, col modello implementativo proposto, non presenta problematiche.

#### Shader side e selezione del texture heap attivo
Il _texture heap_ deve essere disponibile a tutte le shader compatibili con _No Graphics_. La soluzione è quella di, ad ogni dispatch, bindare il _texture heap_ implicitamente, utilizzando il valore specificato dall'utente attraverso la funzione `gpuSetActiveTextureHeap`.

Ogni shader quindi presenterà un bindpoint extra del tipo `device uint64x4* gpuTextureHeap`. L'utente allora per ottenere una texture da un _texture descriptor_, al quale può facilmente accedere, userà funzioni del tipo `GpuTexture` o `GpuTextureRW`, le quali lo trasformeranno in una `metal::texture*<*>`.  
Notiamo che questa trasformazione, in realtà, è un semplice cast, in quanto una texture metal non è altro che un altro indice a 64 bit.

<!-- TODO: Migliora la spiagazione a lato shader ed integra con l'utilizzo di slang. -->

# Progettazione dell'implementazione
<!-- Si è deciso di usare C, nel particolare C89 (anche conosciuto come ANSI C), in quanto la codebase deve restare compatibile sia con ObjectiveC (per interop con Metal), sia con C++ (per un'eventuale futuro interop con DirectX). -->  
<!-- Diversamente da come spesso si pensi, C e C++ sono due linguaggi differenti, che, nella loro crescita negli ultimi quarant'anni, hanno avuto un'evoluzione diversa. -->  
<!-- C++ è un quasi superset di C89, per questo viene scelta questa versione: risulta facile sin dall'inizio creare codice che sia compilabile in entrambi i linguaggi. -->

<!-- ObjectiveC è un superset stretto di C. Questo rimane vero anche per versioni più recenti di C89. Non risultano quindi problemi. -->
Si è deciso di adottare l'utilizzo di C++, mantenendo l'ABI della libreria C-compatible.

Si programmerà in modo data-oriented, ponendo molta enfasi sull'utilizzo della memoria, degli allocatori, etc...
Verranno quindi evitate feature di C++ moderno, preferendo quindi un C++ più in stile C.

Objective-C++ verrà utilizzato per l'integrazione con Metal.

## Scelta del linguaggio di shading
In previsione del futuro, viene scelto, Slang, in quanto è l'unico linguaggio cross-platform che supporta puntatori a device memory. Verrà sviluppata una libreria slang che renderà trasparente l'integrazione con NoGfx.

# Progettazione del testing

# Analisi della fattibilità di integrazione con altre Api di accelerazione grafica

A prima vista, sia DirectX 12, che Vulkan risultano adatti ad essere utilizzati come backend per NoGfx. Infatti sarebbe possibile implementare l'intera api cpu-side in entrambe le piattaforme.

Risulta problematica la situazione quando si parla di linguaggi di shading GPU side: sia HLSL, che GLSL non supportano i device pointers, limitando quindi ciò che è possibile "codificare" con l'api a lato cpu.  
Il problema non è di supporto hardware, in quanto e schede grafiche a moderne supportano tali tipi di puntatori (come si può anche vedere dal funzionamento di Cuda e di OpenCL). Il bytecode Spirv supporta infatti i device pointers attraverso estensioni. Vulkan supporta l'esecuzioni di tali tipi di shader quando la feature `VK_KHR_buffer_device_address` risulta disponibile ([supportata in più dell'80% dei sistemi](https://vulkan.gpuinfo.org/displayextensiondetail.php?extension=VK_KHR_buffer_device_address)).

Se si parla di Vulkan, quindi risolvere questo problema richiederebbe semplicemente l'utilizzo di linguaggi con supporto per tali tipi di puntatori, come Slang, il quale può risultare un'alternativa a GLSL.  
Non vi è invece alcuna alternativa per DirectX 12, in quanto il bytecode DXIL non supporta il concetto di puntatore, rendendo quindi la possibile implementazione non totalmente accurata a quella descritta nell'articolo.

Altre api, come OpenGL, DirectX 11 e precedenti, WebGL e WebGpu risultano troppo limitanti per implementare NoGfx, anche per quanto riguarda il cpu-side.

<!-- TODO: analizzare anche il caso Metal 3, siccome MacOs Tahoe non è ancora molto diffuso -->

# Glossario
amd64 = x86_64
arm64 = aarch64

