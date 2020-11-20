require 'formula'

class VideoProcess < Formula
  homepage 'https://https://github.com/hangoon2/VideoProcess'
  head 'https://https://github.com/hangoon2/VideoProcess', :using => :git

  url 'https://https://github.com/hangoon2/VideoProcess/releases/download/1.0.0/nxptc_28_vd'

  def install
    bin.install 'VideoProcess'
  end

end
