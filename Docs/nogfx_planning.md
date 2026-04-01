# Pianificazione NoGfx Api

# Analisi dei requisiti

Implementare l'api NoGfx in una libreria utilizzabile dal linguaggio di programmazione C:
- La libreria sarà distribuita come set di file oggetto e header C (compatibile con C++).
- Sarà possibile compilare la libreria da sorgenti se necessario. 

L'overhead imposto da tale libreria deve essere il più basso possibile:
- Si cerca un overhead non superiore al 5% rispetto al codice Metal equivalente. In riferimento, MoltenVk,in un caso d'uso generico introduce un overhead di circa il 10%, ma deve emulare un'API molto più grande (Vulkan). // TODO: Find reference, non mi fido troppo di Chatty
- Verranno valutati sia i casi gpu-bound, che cpu-bound per la valutazione della performance

La libreria deve funzionare su hardware Apple dotato di processore Apple Silicon:
- Verrà utilizzata l'API di accelerazione grafica Metal 4, che, grazie alle nuove features introdotte rende la conversione realistica.
- Si targeta quindi macOs 26 Tahoe

# Struttura multi-layer

L'api inizialmente sarà implementata al di sopra di Metal 4, tuttavia è necessario  

## Analisi dell'API e risoluzione delle incrongruenze

### Buffers and memory menagement
Per buffer si intende un blob continuo di memoria acessibile dalla scheda grafica ed, opzionalmente, dalla cpu.

L'api ammette molteplici tipologie di buffer:
  - `MEMORY_DEFAULT`: il buffer risiede nella memoria della scheda grafica, ma è accessibile in scrittura dalla cpu attraverso `cpu mapped gpu pointers`.
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

Si è pensato quindi ad una soluzione in doppio passaggio:
  - 1: Lookup lineare per la risoluzione di puntatori senza offset all'allocazione problematica. Verrà usata una mappa del tipo `map[void*]MTLBuffer`. Se la mappa non contiene un associazione con il puntatore richiesto, allora sarà necessario effettuare il secondo passaggio
  - 2: Ricerca del puntatore con l'offset in uno storage di allocazioni. Si prevede di usare un albero binario basato sui bit del puntatore. L'albero avrà altezza $h <= 48$, in quanto, come amd64, anche in arm64 solo i 48 bit meno significativi vengono utilizzati per codificare il puntatore attuale (diversamente da amd64, i primi 16 bit vengono usati per contenere metadati, che possono essere ignorati). Ci si aspetta quindi una complessità $O(h) = O(48) = O(1)$. La soluzione a due passaggi è necessaria in quanto la ricerca nell'albero ha costante moltiplicativa molto più alta. (TODO: Accennare alla possibilità di analizzare la percentuale di chiamate a `gpuHostDeviceMemory` per determinare se il primo passaggio ha effettivamente senso).

È possibile riutilizzare la mappa `map[void*]MTLBuffer` per l'implementazione della funzione `gpuFree`.

Si nota infine che, per come specificato nell'originario articolo, un buffer `MEMORY_DEFAULT` non dovrebbe permettere operazioni di lettura. Non vi è alcun modo di fare in modo che il _cpu mapped virtual address_ sia accessibile solo in scrittura. La lettura infatti funzionerebbe senza problemi (sebbene la tipologia di buffer `MTLResourceCPUCacheModeWriteCombined` comporti overhead).

#### MEMORY_GPU Buffers
Questi tipi di buffer sono identificati da un unico tipo di puntatori: solo da _gpu virtual pointer_. Segue quindi che la funzione `gpuMalloc(size, MEMORY_GPU)` non ritornerà un _cpu mapped virtual address_, bensì un _gpu virtual address_.

Il corrispettivo più adatto all'implementazione di questo tipo di buffer è un `MTLBuffer` creato con la resource option `MTLStorageModePrivate | MTLResourceTrackingModeUntracked`, accessibile quindi solo dalla scheda grafica.

#### MEMORY_READBACK Buffers
Questi tipi di buffer sono estremamente simili ai buffer di tipologia `MEMORY_DEFAULT`.
Il corrispettivo è un `MTLBuffer` con resource option `MTLStorageModeShared | MTLResourceCPUCacheModeDefaultCache | MTLResourceHazardTrackingModeUntracked`. L'opzione `MTLResourceCPUCacheModeDefaultCache` garantisce una tipologia di gestione della cache più lenta, ma ottimale per la lettura dal buffer.

# Progettazione dell'implementazione

# Progettazione del testing

# Analisi della fattibilità di integrazione con altre Api di accelerazione grafica

A prima vista, sia DirectX 12, che Vulkan risultano adatti ad essere utilizzati come backend per NoGfx. Infatti sarebbe possibile implementare l'intera api cpu-side in entrambe le piattaforme.

Risulta problematica la situazione quando si parla di linguaggi di shading GPU side: sia HLSL, che GLSL non supportano i device pointers, limitando quindi ciò che è possibile "codificare" con l'api a lato cpu.
Il problema non è di supporto hardware, in quanto e schede grafiche a moderne supportano tali tipi di puntatori (come si può anche vedere dal funzionamento di Cuda e di OpenCL). Il bytecode Spirv infatti supporta infatti i device pointers attraverso estensioni. Vulkan supporta l'esecuzioni di tali tipi di shader quando la feature `VK_KHR_device_address` è disponibile.

Se si parla di Vulkan, quindi risolvere questo problema richiederebbe semplicemente l'utilizzo di linguaggi con supporto per tali tipi di puntatori, come Slang, il quale può risultare un'alternativa a GLSL.
Non vi è invece alcuna alternativa per DirectX 12, in quanto il bytecode DXIL non supporta il concetto di puntatore, rendendo quindi la possibile implementazione non totalmente accurata a quella descritta nell'articolo.

Altre api, come OpenGL, DirectX 11 e precedenti, WebGL e WebGpu risultano troppo limitanti per implementare NoGfx, anche per quanto riguarda il cpu-side.

# Glossario
amd64 = x86_64
arm64 = aarch64

