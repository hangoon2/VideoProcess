# VideoProcess

### Dependency
```shell
- opencv 설치
brew install opencv

- ffmpeg 설치
brew install ffmpeg
```

### 설치 시 에러발생
```shell
- brew link error
 . Could not symlink, " " is not writable
 
 => sudo chown -R `whoami`:admin /usr/local/bin
 => sudo chown -R `whoami`:admin /usr/local/share ...
```

### 특정 버전 설치
```shell
git clone -b "태그명" "git 주소"
cd "git에서 받은 바이너리 폴더"
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make -j8
sudo make install
```
