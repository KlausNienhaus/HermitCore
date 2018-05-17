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

file_timings="coldmigration/coldmig_timings.log"
echo "MemoryChunks TimeSpent(us) Config(us) vcpu(us) clock(us) memory(us) pagetablewalk(us) chunktransfer(us) restoretime(us)"  > $file_timings

file_empty="coldmigration/coldmig_uhyve_empty_0,03.log"
file_250="coldmigration/coldmig_uhyve_empty_0,30.log"
file_500="coldmigration/coldmig_uhyve_empty_0,60.log" 
file_1="coldmigration/coldmig_uhyve_empty_1,10.log"
file_2="coldmigration/coldmigE_uhyve_2.log"  
file_4="coldmigration/coldmigE_uhyve_4.log"
file_8="coldmigration/coldmigE_uhyve_8.log"
file_16="coldmigration/coldmigE_uhyve_16.log"  

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
    sleep 3
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=280M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test0 & sleep 2 
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
    sleep 3
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=530M
	
 
    bin/proxy x86_64-hermit/extra/tests/migration_test0 & sleep 2 
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
    sleep 3
    done

counter=0;
until [ $counter -eq $iterations ]; do
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=1050M
	
   
    bin/proxy x86_64-hermit/extra/tests/migration_test0 & sleep 2 
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
    sleep 3
    done


    # HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=127.0.0.1 COMM_ORIGIN=127.0.0.1 HERMIT_MEM=8G bin/proxy x86_64-hermit/extra/tests/migration_test8 