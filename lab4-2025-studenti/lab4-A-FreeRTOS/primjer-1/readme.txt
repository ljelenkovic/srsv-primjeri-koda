Za preuzimanje FreeRTOS jezgre trebao sam skinuti FreeRTOS sa gita sa svim potrebnim modulima. 
Ovo je skinulo sve funkcionalnosti FreeRTOS-a iako sve nisu bile potrebne.  
Sa komandom git clone --recurse-submodules https://github.com/FreeRTOS/FreeRTOS skinuli su sve si potrebni podatci. 
Nakon toga sam napravio Makefile koji u sebi ima ime ulazne C datoteke i izlazne izvršne datoteke. 
Osim toga ima putanju direktorija FreeRTOS-a i svih potrebnih sourceova za prevođenje programa.
