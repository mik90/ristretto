This is a frontend for a kaldi TCP server
should just read in audio from ALSA, then feed it to a server

## TODO
- Buffer audio
- Check for accuracy of decoding, currently see issues

### General Notes
- Move all the configuration into AlsaConfig
- Setup CLI interface for recording 


#### Kaldi server side
- Download aspire chain model with HCLG already compiled
    - my computer doesn't have enough RAM to train the nnet3 t_dnn for librispeech :(
    - It fails when converting a 4-gram arpa to the ConstArpaLm format
    - command: utils/build_const_arpa_lm.sh data/local/lm/lm_fglarge.arpa.gz data/lang_nosp data/lang_nosp_test_fglarge
    - Reading the README.txt did the trick, running the first 1/2 scripts generated the config i needed
        - /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/online.conf.conf
- Maybe put this in a docker container?
- Can do "--device /dev/snd" to share the soundcards
