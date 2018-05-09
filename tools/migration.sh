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
file_250="coldmig_uhyve_250.log"
file_500="coldmig_uhyve_500.log" 
file_1="coldmig_uhyve_1.log"  

#echo "cold migration" >> $file_empty
counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1
	
     
    bin/proxy x86_64-hermit/extra/tests/migration_test0 & sleep 2 
    kill -10 $! & start="$(date -u +%s.%N)"
    start2="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date -u +%s.%N)"
    start3="$(date +%s.%N)"
    END=$(echo "$(date -u +%s.%N) - $start" | bc)
    migtime=$(bc <<< "$stop - $start")
    echo -n "$migtime"  >> $file_empty
    migtime2=$(bc <<< "$stop - $start2")
    echo "      $migtime2"  >> $file_empty
    echo "migration time $migtime"
    echo "migration end $END"
    END2=$(echo "$(date -u +%s.%N) - $start3" | bc) 
    echo "migration end $END2"

    let counter+=1
    sleep 15
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test250 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    start2="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo -n "$migtime"  >> $file_250
    migtime2=$(bc <<< "$stop - $start2")
    echo "      $migtime2"  >> $file_250
    echo "migration time $migtime"
    let counter+=1
    sleep 15
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1
	
 
    bin/proxy x86_64-hermit/extra/tests/migration_test500 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    start2="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo -n "$migtime"  >> $file_500
    migtime2=$(bc <<< "$stop - $start2")
    echo "      $migtime2"  >> $file_500
    echo "migration time $migtime"
    let counter+=1
    sleep 15
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test1 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    start2="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo -n "$migtime"  >> $file_1
    migtime2=$(bc <<< "$stop - $start2")
    echo "      $migtime2"  >> $file_1
    echo "migration time $migtime"
    let counter+=1
    sleep 15
    done
