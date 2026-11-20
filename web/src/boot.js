try {
  const theme = JSON.parse(localStorage.getItem("meadows.settings")).theme;
  if (theme && theme !== "system") {
    document.documentElement.dataset.theme = theme;
  }
} catch {
  document.documentElement.removeAttribute("data-theme");
}
if (new URLSearchParams(location.search).has("embed")) {
  document.documentElement.dataset.embed = "";
}
