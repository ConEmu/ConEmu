$all_total = 0
$per_page = 100
for ($page = 1; ; $page++) {
  # Outer bar: page counter (total number of pages is unknown)
  Write-Progress -Id 1 -Activity "GitHub releases" -Status "Fetching page $page..."
  $url = "https://api.github.com/repos/ConEmu/ConEmu/releases?per_page=$per_page&page=$page"
  $rels = (conemuc -download $url - 2>$null)
  $rel = @((ConvertFrom-Json $rels) | ForEach-Object { $_ })
  if ($rel.Count -eq 0) { break }

  $i = 0
  foreach ($r in $rel) {
    $i++
    # Inner bar: progress within the current page
    Write-Progress -Id 2 -ParentId 1 -Activity "Page $page" `
      -Status $r.name -PercentComplete ($i * 100 / $rel.Count)
    $total = 0
    foreach ($a in $r.assets) { $total += $a.download_count }
    Write-Host "$($r.name): $total"
    $all_total += $total
  }
}
Write-Progress -Id 2 -Activity "Page" -Completed
Write-Progress -Id 1 -Activity "GitHub releases" -Completed
Write-Host "Finish, total downloads: $all_total"
