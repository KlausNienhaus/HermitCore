if [ "$#" -ne 3 ]; then
    if [ "$#" -eq 2 ]; then
        iterations=1;
    else
        echo "Illegal number of parameters"
        echo "    usage:"
        echo "    migration <client-ip> <server-ip> <iterations>"
        exit 1;
    fi
else
    iterations="$3";
fi

file_empty="coldmig_uhyve_empty.log"

counter=0;
until [ $counter -eq $iterations ]; do
     export HERMIT_ISLE=uhyve HERMIT_CHECKPOINT=1 PROXY_COMM=client
     start_time=$SECONDS
     /usr/bin/time -f %e -o $file_empty -a bin/proxy x86_64-hermit/extra/tests/migration_test
     if [ $? -ne 0 ]; then
        echo "migration proccess encountered an error";
        exit 1;
     fi  
     sleep 0.5;
     export HERMIT_ISLE=uhyve HERMIT_CHECKPOINT=1 PROXY_COMM=server
     /usr/bin/time -f %e -o $file_empty -a bin/proxy x86_64-hermit/extra/tests/migration_test
     if [ $? -ne 0 ]; then
        echo "migration proccess encountered an error";
        exit 1;
     fi 
     sleep 0.5;
     let counter+=1
done

elapsed_time=$(($SECONDS-$start_time))
echo "Time since start $elapsed_time"