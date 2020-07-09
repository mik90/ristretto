#!/bin/bash
set -e

build()
{
    echo "Running kaldiTcpServer.sh::build()"
    cd ../kaldi/src
    make
    cd -
}

start_server()
{
    echo "Running kaldiTcpServer.sh::start_server()"

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

    if [ -f "/opt/ristretto/logs/kaldi-server.log" ]; then
        rm /opt/ristretto/logs/kaldi-server.log
    fi
    pushd /opt/kaldi
    if [ "$DEBUG" != "YES" ]; then
        # Write to both the terminal and kaldi.log
        ./src/online2bin/online2-tcp-nnet3-decode-faster $ARGS  2>&1 | tee /opt/ristretto/logs/kaldi-server.log
    else
        # Debugging ~enabled~
        gdb --quiet -ex run --args ./src/online2bin/online2-tcp-nnet3-decode-faster $ARGS 2>&1 | tee /opt/ristretto/logs/kaldi-server.log
    fi
    popd
}

main()
{
    echo "Running kaldiTcpServer.sh"
    cd /opt/kaldi
    if [ "$SKIP_BUILD" != "YES" ]; then
        build
    fi
    if [ "$SKIP_RUN" != "YES" ]; then
        start_server
    fi
}

main