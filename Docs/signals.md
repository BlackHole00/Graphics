# Signaling and waiting brainstorm

## Compute pass
Un Command buffer può avere più compute pass (e render pass).

## Primitivi Metal disponibili

Intrapass barriers:
  - Nope

Infrapass barriers:
  - non funzionano tra code

Fence:
  - funzionano tra code (ma sconsigliato)
  - richiedono che i command buffer segnalatori vengano submittati prima dei buffer aspettatori. <== Limitante

Event:
  - funzionano tra code
  - indipendenti dell'ordine wait-signal
  - ogni event deve usare un valore numerico monotonicamente crescente
  - lavora sul lavoro submitted nella coda. <== wait a livello di coda, non di command buffer (più broad del necessario, ma hey!)

SharedEvent:
  - perfetto equivalente per GpuSemaphore.

Vengono usati gli Event.

# Encoding
Siccome gli event lavorano a livello di coda e non di command buffer, per utilizzarli è necessario effettuare un commit sulla queue.  
Siccome effettuare commit impliciti non è una soluzione molto clean, sono state analizzate le seguenti strategie:
- ogni command buffer ha una coda associata. I comandi vengono implicitamente committati sulla coda, ma l'esecuzione del primo è limitata dall'attesa di un evento di un MTLSharedEvent, segnalato al momento del commit.
  - NOTA: Metal permette ad un processo di creare 65535 code (anche se comincia a tirare warning dopo le 63)
  - PRO: È possibile codificare i comandi direttamente su primitivi Metal, senza dover mantenere stato interno alla libreria.
  - CONTRO: Vengono sprecate code (bisogna imporre un limite massimo al numero di command buffer codificabili in parallelo)
  - CONTRO: Ogni command buffer deve essere submittato prima o poi (se no una coda non può esssere utilizzata)
- ogni command buffer mantiene una coda di comandi completati e di eventi che deve segnalare o aspettare. Alla chiamata di gpuCommit i comandi verranno codificati nella coda specificata, con i giusti signal e wait (la coda verrà acquisita in maniera esclusiva).

# Operazioni supportate
Le uniche operazioni supportate sono:
  - signal: SIGNAL_ATOMIC_MAX
  - wait:  OP_GREATER_EQUAL, mask 0xFF..FF
Non è possibile scrivere manualmente il valore del segnale.
1:1 con la funzionalità di MTLEvent + Facilmente emulabile in Vulkan (timeline semaphores).


