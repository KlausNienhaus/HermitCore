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



mkdir coldmigration

file_empty="coldmigration/coldmig_uhyve_0,00.log"
file_250="coldmigration/coldmig_uhyve_0,25.log"
file_500="coldmigration/coldmig_uhyve_0,50.log" 
file_1="coldmigration/coldmig_uhyve_1.log"
file_2="coldmigration/coldmig_uhyve_2.log"  
file_4="coldmigration/coldmig_uhyve_4.log"
file_8="coldmigration/coldmig_uhyve_8.log"
file_16="coldmigration/coldmig_uhyve_16.log"  

#echo "cold migration" >> $file_empty
#waittonext = 6
counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=30M
	
     
    bin/proxy x86_64-hermit/extra/tests/migration_test0 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_empty
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 5
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=300M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test250 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"	
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_250
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 5
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=600M
	
 
    bin/proxy x86_64-hermit/extra/tests/migration_test500 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_500
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 5
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=1100M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test1 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_1
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 7
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=2500M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test2 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_2
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 7
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=4500M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test4 & sleep 2 
    kill -10 $! & start="$(date +%s.%N)"
    if [ $? -ne 0 ]; then
       echo "migration proccess encountered an error";
       exit 1;	
    fi 
    wait
    stop="$(date +%s.%N)"
    migtime=$(bc <<< "$stop - $start")
    echo "$migtime"  >> $file_4
    echo "migration $counter time $migtime"
    let counter+=1
    sleep 10
    done


    # HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=127.0.0.1 COMM_ORIGIN=127.0.0.1 HERMIT_MEM=8G bin/proxy x86_64-hermit/extra/tests/migration_test8 