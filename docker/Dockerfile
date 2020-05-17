FROM kaldiasr/kaldi:gpu-latest

# Need to unzip the trained aspire model with HCLG into the egs/aspire/s5/exp dir
# Don't want to store this in the repo but can't access it when it's in the /support-kaldi folder
COPY 0001_aspire_chain_model_with_hclg.tar.bz2 /opt/kaldi
RUN tar -jxvf /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2 -C /opt/kaldi/egs/aspire/s5/
RUN rm /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2

WORKDIR /opt/kaldi/egs/aspire/s5
# The README.txt in this directory gives some useful info
RUN steps/online/nnet3/prepare_online_decoding.sh \
    --mfcc-config conf/mfcc_hires.conf data/lang_chain exp/nnet3/extractor exp/chain/tdnn_7b exp/tdnn_7b_chain_online
RUN utils/mkgraph.sh --self-loop-scale 1.0 data/lang_pp_test exp/tdnn_7b_chain_online exp/tdnn_7b_chain_online/graph_pp
# I've been having problems with either not recording in 8 KHz or Kaldi not recognizing my sampling rate
# I'd assume it's an issue on my end. Enabling downsampling will 'fix' it
RUN echo "--allow_downsample=true" >> /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/mfcc.conf

ENV force_color_prompt=yes