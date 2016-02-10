# ZSDD
##Zero-suppressed Sentential Decision Diagrams
Sample software of Zero-suppressed Sentential Decision Diagrams (ZSDDs), which compiles a CNF/DNF into a ZSDD. Please see the paper for the details of ZSDDs.


##Usage
```
zsdd [-c .] [-d .] [-v .] [-R .] [-S .]  [-h]
    -c FILE        set input CNF file
    -d FILE        set input DNF file
    -v FILE        set input VTREE file (default is a right-linear vtree)
    -e             use zsdd without implicit partitioning
    -R FILE        set output ZSDD file
    -S FILE        set output ZSDD (dot) file
    -h             show help message and exit
```    

##Reference
Masaaki Nishino, Norihito Yasuda, Shin-ichi Minato, and Masaaki Nagata: "Zero-suppressed Sentential Decision Diagrams," In Proc. of the 30th AAAI Conference on Artificial Intelligence (AAAI2016), Feb. 2016.
