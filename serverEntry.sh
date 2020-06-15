#!/bin/bash
set -e

run_protoc()
{
    echo "Running serverEntry.sh::run_protoc()"
    cd /opt/ristretto
    ./runProtoc.sh
}

run_build()
{
    echo "Running serverEntry.sh::run_build()"
    pushd /opt/ristretto/build
    cmake .. -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --parallel $(nproc)
    popd
}


start_server()
{
    echo "Running serverEntry.sh::start_server()"

    ulimit -c unlimited

    # Export directory, shortened to 'exp' in Kaldi
    EXP="/opt/kaldi/egs/aspire/s5/exp"
    MODEL="${EXP}/chain/tdnn_7b/final.mdl"
    HCLG="${EXP}/tdnn_7b_chain_online/graph_pp/HCLG.fst"
    WORDS="${EXP}/tdnn_7b_chain_online/graph_pp/words.txt"
    CONFIG="${EXP}/tdnn_7b_chain_online/conf/online.conf" 
    # Make sure all of the files exist
    for conf in $MODEL $HCLG $WORDS $CONFIG; do
        test -f ${conf} || { echo "${conf} does not exist"; exit 1; }
    done

    ARGS=" --verbose=1 --frames-per-chunk=20 \
    --extra-left-context-initial=0 --frame-subsampling-factor=3 --config=${CONFIG} \
    --min-active=200 --max-active=7000 \
    --beam=15.0 --lattice-beam=6.0 --acoustic-scale=1.0 \
    --port-num=5050 ${MODEL} ${HCLG} ${WORDS}"

    pushd /opt/ristretto

    if [ "$DEBUG" != "YES" ]; then
        ./build/bin/RistrettoServer $ARGS
    else
        # Debugging ~enabled~
        gdb --quiet -ex run --args ./build/bin/RistrettoServer $ARGS
    fi
    popd
}

main()
{
    echo "Running serverEntry.sh"
    cd /opt/ristretto
    if [ "$SKIP_PROTOC" != "YES" ]; then
        run_protoc
    fi
    if [ "$SKIP_BUILD" != "YES" ]; then
        run_build
    fi
    if [ "$SKIP_RUN" != "YES" ]; then
        start_server
    fi
}

main