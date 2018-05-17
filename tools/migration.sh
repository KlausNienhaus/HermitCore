if [ "$#" -ne 4 ]; then
    check_dir="checkpoint"
    if [ "$#" -ne 3 ]; then
        if [ "$#" -eq 2 ]; then
            iterations=1;
        else
            echo "Illegal number of parameters"
            echo "    usage:"
            echo "    ./migration.sh <client/origin-ip> <server/destination-ip> <iterations> <checkpoint-dir>"
            exit 1;
        fi
    else
        iterations="$3";
    fi
else
    iterations="$3";
    check_dir="$4"
fi

mkdir offlinemigration

file_timings="offlinemigration/offlinemig_timings.log"
echo "MemoryChunks TimeSpent(us) Config(us) vcpu(us) clock(us) memory(us) pagetablewalk(us) chunktofile(us) file_close_spent(us) filetransfer(us) otherfilestransfer(us) memfiletransfer(us) restoretime(us)"  > $file_timings

file_empty="offlinemigration/offlinemig_uhyve_0,00.log"
file_250="offlinemigration/offlinemig_uhyve_0,25.log"
file_500="offlinemigration/offlinemig_uhyve_0,50.log" 
file_1="offlinemigration/offlinemig_uhyve_1.log"
file_2="offlinemigration/offlinemig_uhyve_2.log"  
file_4="offlinemigration/offlinemig_uhyve_4.log"
file_8="offlinemigration/offlinemig_uhyve_8.log"
file_16="offlinemigration/offlinemig_uhyve_16.log"  

#echo "cold migration" >> $file_empty
#waittonext = 6
counter=0;
until [ $counter -eq $iterations ]; do
    rm -rf $check_dir
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=30M HERMIT_CHECKDIR=$check_dir
	
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
    rm -rf $check_dir
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=300M HERMIT_CHECKDIR=$check_dir
	
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
    sleep 3
    done

counter=0;
until [ $counter -eq $iterations ]; do
    rm -rf $check_dir
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=600M HERMIT_CHECKDIR=$check_dir
	
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
    sleep 3
    done

counter=0;
until [ $counter -eq $iterations ]; do
    rm -rf $check_dir
    export HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=$2 COMM_ORIGIN=$1 HERMIT_MEM=1100M HERMIT_CHECKDIR=$check_dir
   
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
    sleep 3
    done


    # HERMIT_ISLE=uhyve PROXY_COMM=client COMM_DEST=127.0.0.1 COMM_ORIGIN=127.0.0.1 HERMIT_MEM=8G bin/proxy x86_64-hermit/extra/tests/migration_test8 
