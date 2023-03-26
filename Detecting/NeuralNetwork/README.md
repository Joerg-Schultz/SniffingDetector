# Neural Network for Sniffing detection
## DVC Setup
Remote storage on google drive.
```shell
conda install -c conda-forge dvc-gdrive
# in dir SniffingDetector
dvc init
dvc add .\Detecting\NeuralNetwork\data\raw
git add 'Detecting\NeuralNetwork\data\raw.dvc'
dvc remote add -d storage gdrive://[ID of google drive folder]
```
Now copy your audio files into the data directory.
```shell
dvc add .\data\raw\
git add 'data\raw.dvc'
git commit -m 'First Audio data'
git tag -a 'v1' -m 'primary dataset'
dvc push
```