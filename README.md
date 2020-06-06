## Ristretto
Client/Server programs for online decoding using Kaldi's nnet3 decoder. Uses the fisher-english data set.
Both the client and server have Dockerfiles and there is a docker-compomse that mounts volumes on the current repository for quick changes. 
Client/Server communication is done with gRPC. As a note, this is nowhere near done.


------------------------
## TODO
- Make thread-safe nnet3 API that will be passed to CallData, can start out as a simple/naive implementation
    - CallData has its own state per call
- Send audio chunks to Kaldi, what duration though?
- Logic for left context?
- Should this use a streaming RPC or a normal message based setup?
    - Start out as traditional setup but switch to streaming since audio can easily be streamed and the transcript may also
      be able to be streamed

------------------------
### Notes
#### Kaldi server side
- Download aspire chain model with HCLG already compiled
    - my computer doesn't have enough RAM to train the nnet3 t_dnn for librispeech :(
    - It fails when converting a 4-gram arpa to the ConstArpaLm format
    - command: utils/build_const_arpa_lm.sh data/local/lm/lm_fglarge.arpa.gz data/lang_nosp data/lang_nosp_test_fglarge
    - Reading the README.txt did the trick, running the first 1/2 scripts generated the config i needed
        - /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/online.conf.conf
- Maybe put this in a docker container?
- Can do "--device /dev/snd" to share the soundcards

#### Clang-format
- Using 3 versions, one in container, one on Arch, one on Gentoo
- Gentoo is 9.0.1
- Try to find oldest setup
- Based on LLVM, changed to:
    - TabWidth = 4
    - PointerAlignment = Left
    - ColumnLimit = 100
    - BraceWrapping: AfterControlStatement = true