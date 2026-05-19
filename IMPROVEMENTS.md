Resumen de mejoras implementadas
=================================

Este documento resume las mejoras aplicadas a los archivos `mpi_static.cpp` y `mpi_dynamic.cpp` (proyecto MPI). Las modificaciones mantienen las estrategias originales (estática por bloques de filas y dinámica) y se centran en robustez, medición y comunicación eficiente.

mpi_static.cpp
--------------

Cambios realizados:

- Reemplazo de `MPI_Gather` por `MPI_Reduce` (suma):
  - Antes: se reunían todos los `localCount` en root con `MPI_Gather` y luego se sumaban en el proceso 0.
  - Ahora: se usa `MPI_Reduce(..., MPI_SUM, ...)` para obtener `totalCount` directamente.
  - Beneficio: menor uso de memoria en root, menor complejidad y menos tráfico (semántica de reducción más natural para sumas).

- Eliminación de la barrera redundante:
  - Se dejó una única `MPI_Barrier` antes de iniciar el cómputo.
  - Beneficio: menos sincronización innecesaria (menor overhead).

- Medición por proceso y reducciones de tiempo:
  - Cada proceso mide su tiempo de procesamiento (`localTimeMs`) usando `MPI_Wtime()`.
  - Se reducen `max/min/sum` de tiempo con `MPI_Reduce` para obtener `max`, `min` y `avg` en root.
  - Beneficio: métricas más precisas para analizar equilibrio de carga y escalado.

- Evitar trabajo en procesos sin filas asignadas:
  - Si `localRows == 0`, el proceso no realiza cómputo y devuelve `localCount = 0`.
  - Beneficio: evita llamadas inútiles y mediciones engañosas cuando `P > M`.

- Mensajería y salida uniforme:
  - Sólo el proceso root imprime encabezados y los resultados agregados.
  - Beneficio: salida ordenada y menos ruido en logs.

Notas y recomendaciones adicionales:
- Se mantuvo la generación on-the-fly mediante `generateValue(...)` para reducir memoria.
- Para portabilidad y claridad se recomienda el uso de tipos explícitos (`int64_t`, `uint32_t`) y `MPI_INT64_T`/`MPI_UINT32_T` si se desea.

mpi_dynamic.cpp
---------------

Cambios realizados:

- Envío seguro de tareas: ahora `sendTask` transmite un arreglo `int buf[2]` (`startRow`, `rowCount`) en lugar de enviar un `struct Task` directamente.
  - Antes: se hacía `MPI_Send(&task, 2, MPI_INT, ...)` pasando el `struct` (cuyo layout podría generar dudas de portabilidad).
  - Ahora: se construye un `int buf[2]` y se envía explícitamente.
  - Beneficio: evita dependencias implícitas del `struct` (alineamiento/padding) y es portable entre compiladores/archivos.

- Recepción explícita en workers:
  - Los workers reciben dos `int` y validan `rowCount <= 0` para terminar.
  - Beneficio: claridad y menor probabilidad de errores por differences en `struct` layout.

- Medición por proceso (acumulada por chunk):
  - Los workers miden el tiempo por chunk y acumulan en `localTimeMs`.
  - Root reduce `max/min/sum` para obtener `max/min/avg` y mostrar estadísticas.
  - Beneficio: diagnóstico fino de latencias y balance dinámico.

- Mantener una sola `MPI_Barrier`:
  - Igual que en la versión estática, se evita sincronización redundante.

- Manejo de impresiones y errores:
  - Root sigue siendo el único que imprime encabezados y resultados agregados; workers permanecen silenciosos salvo logs de depuración (si se habilitan más adelante).
  - Beneficio: salida más limpia y centralizada.

- Medición en el caso de single-process:
  - Si `size == 1` se mide el tiempo del cómputo secuencial para reportarlo en `localTimeMs`.


Resumen de beneficios globales
-----------------------------

- Menor uso de memoria y lógica simplificada al usar `MPI_Reduce` para sumar conteos.
- Menos sincronización innecesaria (una sola `MPI_Barrier`).
- Métricas de rendimiento más útiles (max/min/avg) para analizar balance y escalado.
- Comunicación de tareas más portátil y robusta en la versión dinámica.
- Salida centralizada en root para facilitar interpretación de resultados.
