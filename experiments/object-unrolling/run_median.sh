EXEC=$1 ;
ARGS="$2" ;
if [ $# -ge 3 ] ; then
    ITERS="$3" ;
else
    ITERS=10 ;
fi

TMP_FILE=`mktemp` ;
> ${TMP_FILE} ;
for RUN_IDX in {1..10} ;
do
    ./${EXEC} ${ARGS} | grep -s "EXEC" | awk '{ print $3 }' >> ${TMP_FILE} ;
    ./${EXEC} ${ARGS} | grep -s "EXEC" | awk '{ print $3 }' ;
done

sort -n ${TMP_FILE} | awk '{ count[i++] = $1 }
                            END { if (i % 2) {
                                print count[(i+1) / 2];
                              } else {
                                print (count[(i/2)] + count[(i/2)+1]) / 2.0;
                              }
                            }' ;
