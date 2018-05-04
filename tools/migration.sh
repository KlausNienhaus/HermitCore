if [ "$#" -ne 3 ]; then
    if [ "$#" -eq 2 ]; then
        iterations=1;
    else
        echo "Illegal number of parameters"
        echo "    usage:"
        echo "    ./migration.sh <client-ip> <server-ip> <iterations>"
        exit 1;
    fi
else
    iterations="$3";
fi

file_empty="coldmig_uhyve_empty.log"

#echo "cold migration" >> $file_empty
counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1
	
     
    bin/proxy x86_64-hermit/extra/tests/migration_test & sleep 1 
    kill -10 $! & start="$(date +%s.%N)"
    start2="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo -n "$migtime"  >> $file_empty
    migtime2=$(bc <<< "$stop - $start2")
    echo "      $migtime2"  >> $file_empty
    echo "migration time $migtime"
    let counter+=1
    sleep 5
    done