# SSOO-II
## cambios
El proceso inicial se encargará de preparar todas las variables y recursos IPC de la aplicación. Este proceso creará 32 procesos hijos. Cada proceso hijo, representa a una persona. Los nombres de las personas, junto con las abreviaturas usadas para referirse a ellas, se muestran en la siguiente tabla:
|  |  |  |  |
|---|---|---|---|
| (A) Ana | (I) Ignacio | (a) Alberto | (i) Ildefonsa |
| (B) Benito | (J) Juan| (b) Bonifacia | (j) Josefa |
| (C) Carla | (L) Laura | (c) Conrado | (l) Luis |
| (D) Daniela | (M) Manuel | (d) David | (m) María |
| (E) Emilio | (N) Nicanor | (e) Eulalia | (n) Natalia |
| (F) Flor | (O) Olvido | (f) Federico | (o)  Olegario |
| (G) Gonzalo | (P) Pedro | (g) Gracia | (p) Pilar |
| (H) Honoria | (R) Rosa | (h) Hilario | (r) Ramón |


En la asignatura objeto de la páctica hay cuatro grupos (I, II, III y IV) y se ha establecido que, al comienzo, los grupos se formarán en atención a la inicial del nombre de la persona. La partición se ha efectuado siguiendo el criterio: A-D, E-H, I-M, N-R, para los grupos I, II, III y IV, respectivamente. Cada proceso, según nace, ha de ir al grupo que le corresponde.

La vida de los procesos transcurre plácida y tranquila. Duermen un tiempo y, pasado este, deciden que quieren cambiar de grupo. Se lo solicitan al proceso padre, que por cierto se llama Zacarías(Z). Z, viendo las solicitudes que hay en cada momento, decide conceder el cambio inmediatamente o hacerlo más tarde. El criterio que se mantiene es que cualquier cambio no debe implicar variación en el número de alumnos por grupo. Por ejemplo, si hay un alumno que quiere ir del grupo I al II y otro que quiere ir del II al I, es un cambio correcto. Si hay un alumno que quiere ir del I al II, otro del II al III y otro del I al IV, es un cambio incorrecto. 