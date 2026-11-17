export function formatDuration(ms: number) {
  const totalSeconds = Math.floor(ms / 1000);
  const seconds = totalSeconds % 60;
  const minutes = Math.floor(totalSeconds / 60) % 60;
  const hours = Math.floor(totalSeconds / 3600) % 24;
  const days = Math.floor(totalSeconds / 86400);

  if (totalSeconds < 60) {
    return `${seconds} s`;
  }

  if (totalSeconds < 3600) {
    return `${minutes} min ${seconds} s`;
  }

  if (totalSeconds < 86400) {
    return `${hours} h ${minutes} min`;
  }

  return `${days} d ${hours} h`;
}

export function formatBytes(bytes: number) {
  if (bytes >= 1048576) {
    return `${(bytes / 1048576).toFixed(1)} MB`;
  }

  if (bytes >= 1024) {
    return `${Math.round(bytes / 1024)} KB`;
  }

  return `${bytes} B`;
}
