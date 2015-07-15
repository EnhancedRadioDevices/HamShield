chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create("window.html", {
    "bounds": {
        "width": 685,
        "height": 800
    }
    });
});

  $(function() {
    $( "#tabs" ).tabs();
  });

