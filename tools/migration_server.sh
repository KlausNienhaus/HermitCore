if [ "$#" -ne 3 ]; then
    if [ "$#" -eq 2 ]; then
        iterations=1;
    else
        echo "Illegal number of parameters"
        echo "    usage:"
        echo "    ./migration_server.sh <client/origin-ip> <server/destination-ip> <iterations>"
        exit 1;
    fi
else
    iterations="$3";
fi
export HERMIT_ISLE=uhyve PROXY_COMM=server COMM_DEST=$2 COMM_ORIGIN=$1
counter=0;
until [ $counter -eq $iterations ]; do
    rm -rf checkpoint
	
    bin/proxy x86_64-hermit/extra/tests/migration_test 
    wait

    let counter+=1
    done
