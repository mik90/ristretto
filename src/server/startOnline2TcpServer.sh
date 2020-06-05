#!/bin/bash

# Before doing this, you need to train/download a model that will be read in as .mdl, .txt, and .fst files

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

echo "============================================================================"
CMD="/opt/kaldi/src/online2bin/online2-tcp-nnet3-decode-faster --verbose=1 --frames-per-chunk=20 \
 --extra-left-context-initial=0 --frame-subsampling-factor=3 --config=${CONFIG}  \
 --min-active=200 --max-active=7000 \
 --beam=15.0 --lattice-beam=6.0 --acoustic-scale=1.0 \
 --port-num=5050 ${MODEL} ${HCLG} ${WORDS}"

# Run the command
${CMD}
