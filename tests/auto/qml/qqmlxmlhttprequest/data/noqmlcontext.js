(function(url, resultCollector) {
        var x = new XMLHttpRequest;
        x.open("GET", url);
        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE)
                resultCollector.responseText = x.responseText
        }
        x.send()
})
