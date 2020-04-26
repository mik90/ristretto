This is a frontend for a kaldi TCP server
should just read in audio from ALSA, then feed it to a server

Currently reaching an error upon connect:
    - ERROR (online2-tcp-nnet3-decode-faster[5.5.0~1-e5a5]:DecodableNnetLoopedOnlineBase():decodable-online-looped.cc:46) Input feature dimension mismatch: got 13 but network expects 40
    - This happens when it loads the acoustic model
    - apparently i'm using online_nnet2_decoding.conf

## TODO
- Download aspire chain model with HCLG already compiled
    - my computer doesn't have enough RAM to train the nnet3 t_dnn for librispeech :(
    - It fails when converting a 4-gram arpa to the ConstArpaLm format
    - command: utils/build_const_arpa_lm.sh data/local/lm/lm_fglarge.arpa.gz data/lang_nosp data/lang_nosp_test_fglarge
- Maybe put this in a docker container?
- Can do "--device /dev/snd" to share the soundcards