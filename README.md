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
