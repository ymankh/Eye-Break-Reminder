$sizes = @(16, 24, 32, 48)
$images = @()

Add-Type -AssemblyName System.Drawing

foreach ($size in $sizes) {
    $bitmap = New-Object System.Drawing.Bitmap $size, $size
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::Transparent)

    $blue = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(36, 99, 185))
    $white = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $green = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(34, 149, 97))
    $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::White), ([Math]::Max(2, [int]($size / 11)))

    $pad = [Math]::Max(1, [int]($size / 12))
    $graphics.FillEllipse($blue, $pad, $pad, $size - ($pad * 2), $size - ($pad * 2))

    $center = $size / 2
    $radius = $size * 0.28
    $graphics.DrawEllipse($pen, $center - $radius, $center - $radius, $radius * 2, $radius * 2)
    $graphics.DrawLine($pen, $center, $center, $center, $center - ($size * 0.17))
    $graphics.DrawLine($pen, $center, $center, $center + ($size * 0.13), $center + ($size * 0.09))
    $graphics.FillEllipse($green, $size * 0.58, $size * 0.58, $size * 0.23, $size * 0.23)

    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $images += ,@($size, $stream.ToArray())

    $graphics.Dispose()
    $bitmap.Dispose()
}

$output = New-Object System.IO.FileStream "app.ico", ([System.IO.FileMode]::Create), ([System.IO.FileAccess]::Write)
$writer = New-Object System.IO.BinaryWriter $output
$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$images.Count)

$offset = 6 + ($images.Count * 16)
foreach ($image in $images) {
    $size = [int]$image[0]
    $bytes = [byte[]]$image[1]
    $writer.Write([byte]$size)
    $writer.Write([byte]$size)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]$bytes.Length)
    $writer.Write([UInt32]$offset)
    $offset += $bytes.Length
}

foreach ($image in $images) {
    $writer.Write([byte[]]$image[1])
}

$writer.Dispose()
$output.Dispose()
Write-Host "Created app.ico"
