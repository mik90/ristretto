[![Build Status](https://travis-ci.org/mik90/ristretto.svg?branch=develop)](https://travis-ci.org/mik90/ristretto)

This is a frontend for a kaldi TCP server
should just read in audio from ALSA, then feed it to a server

## TODO
- Need to write test setup for the audio consumption logic
- Send audio chunks to Kaldi, what duration though?
- Logic for left context?

#### Kaldi server side
- Download aspire chain model with HCLG already compiled
    - my computer doesn't have enough RAM to train the nnet3 t_dnn for librispeech :(
    - It fails when converting a 4-gram arpa to the ConstArpaLm format
    - command: utils/build_const_arpa_lm.sh data/local/lm/lm_fglarge.arpa.gz data/lang_nosp data/lang_nosp_test_fglarge
    - Reading the README.txt did the trick, running the first 1/2 scripts generated the config i needed
        - /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/online.conf.conf
- Maybe put this in a docker container?
- Can do "--device /dev/snd" to share the soundcards
