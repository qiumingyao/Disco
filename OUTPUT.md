# Disco Assembler Output

**Disco** is overlap-layout-consensus (OLC) metagenome assembler for handling large read sets. The output format of the various final and intermediate files generated by the the omega2 assembler are described below. All output files of the assembler are prefixed with a user given string. For the document below we assume that string to be "testA". 

## Final Result Files
Each assembly has 2 directories named graph and assembly. The graph directory contains the initial overlap graph and the assembly directory has the final assembly.  

1. graph/testA_ReadIDMap.txt : The disco assembler does not maintain the initial read names from the input fasta/fastq files. The reads in each input file are assigned a sequential identifier. This file contains the range of read identifiers assigned to each file. The files are classified as paired or single read files. These read identifiers are used throughout the assembly process. 

2. graph/testE_cointainedReads.txt : This file contains the duplicate and contained reads. The file format is as follows:
    * First column is the id of contained read (subread) followed by a <TAB>.
    * Second column is the id of containing read (superread) followed by a <TAB>.
    * Third column is a comma separated decuple. The fields are (orientation, overlap length, substitutions, edits, contained read length, contained read overlap start, contained read overlap end, containing read length, containing read overlap start, containing read overlap end). The orientation can be one of the 4 values 0,1,2,3.
    
```

        	 * orient 0
                    >--------MMMMMMMMMMMMMMM*******------> read1      M means initial match found by hash table
                             MMMMMMMMMMMMMMM*******>       read2      * means to be checked by comparison 
 
                * orient 2
                    >---*****MMMMMMMMMMMMMMM*******------> read1
                             MMMMMMMMMMMMMMM*****<         Reverese complement of read2

                * orient 1
                    >---*****MMMMMMMMMMMMMMM-------------> read1      
                       >*****MMMMMMMMMMMMMMM       	    read2       

                * orient 3
                    >---*****MMMMMMMMMMMMMMM-------------> read1
                        <****MMMMMMMMMMMMMMM               Reverse Complement of Read2

```

3. testA_contigsFinal.fasta : This file contains the assembled contig sequences in fasta format. All contigs greater than 500bp are written into this file. The name of the contig is the string "contig_" followed by a sequentially ordered number of 10 digits. The description for each sequences provides an ordered pair of read ids (read1, read2). These two reads are the first and the last read of the contig respectively. In addition, the length of the contig is included in the description.

4. assembly/testA_contigFinalEdges.txt : This file contains the edges in the assembled contig. Each contig corresponds to a composite edge in the overlap graph. A composite edge is a sequence of read ids with overlap and orientation information. This file can be used for scaffolding purposes. The format of the file is:
     * Each line is one composite edge.
     * First is the contig identifier to match against the contig sequence file.  
     * Second column is the id of first read(source) in the composite edge followed by a <TAB>. 
     * Third column is the id of second read(destination) in the composite edge followed by a <TAB>. 
     * The fourth column is a comma separated quintuple (orientation, first offset, composite edge length, substitutions, edits):
        * Orientation can be one of the 4 values 0,1,2,3.
            * 0 = u<-----------<v		reverse of u to reverse of v
            * 1 = u<----------->v		reverse of u to forward of v
            * 2 = u>-----------<v		forward of u to reverse of v 
            * 3 = u>----------->v		forward of u to forward of v
        * First offset is the offset from the start of the source to the first start of overlap. 
        * Composite edge length is length of the whole composite edge in base pairs. 
        * Substitutions is the the number of substitutions done in the overlap calculation. This is always 0 for now. 
        * Edits is the the number of edits done in the overlap calculation. This is always 0 for now.
    * The fifth column is a list of triplets (read id, orientation, offset)
             *  Read id : Identifier of the read.
             *  Orientation : 0 if forward or 1 is reverse
             *  Offset : Offset from the start of the read to the next overlap. 
```

                    *************MMMMMMMMMMM>

                                 MMMMMMMMMMM**********>

                    |---Offset---|
```

5. assembly/testA_contigEdgeCoverageFinal.txt : This file contains all the base pair coverage values of all the contigs.

6. testA_scaffoldsFinal.fasta : This file contains the assembled scaffold sequences in fasta format.

7. assembly/testA_scaffoldFinalEdges.txt : This file contains the assembled scaffold edges in format described above in point 4.

8. assembly/testA_scaffoldEdgeCoverageFinal.txt : This file contains the assembled scaffold coverage values in format described above in point 5.