//  file: pebble-js-app.js
//  auth: Matthew Clark, SetPebble

// change this token for your project
var setPebbleToken = 'C9UH';
var parametri;

Pebble.addEventListener('ready', function(e) {
});
Pebble.addEventListener('appmessage', function(e) {
  key = e.payload.action;
  if (typeof(key) != 'undefined') {
    var settings = localStorage.getItem(setPebbleToken);
    if (typeof(settings) == 'string') {
      try {
        Pebble.sendAppMessage(JSON.parse(settings));
				console.log('inviato: '+JSON.parse(settings));
      } catch (e) {
      }
    }
    var request = new XMLHttpRequest();
    request.open('GET', 'http://x.SetPebble.com/api/' + setPebbleToken + '/' + Pebble.getAccountToken(), true);
    request.onload = function(e) {
      if (request.readyState == 4)
        if (request.status == 200)
          try {
            Pebble.sendAppMessage(JSON.parse(request.responseText));
						console.log('send:'+ JSON.parse(request.responseText));
          } catch (e) {
          }
    }
    request.send(null);
  }
});
Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://x.SetPebble.com/' + setPebbleToken + '/' + Pebble.getAccountToken());
	console.log('URL aperto');
});
Pebble.addEventListener('webviewclosed', function(e) {
  console.log('webclosed event');
	if ((typeof(e.response) == 'string') && (e.response.length > 0)) {
		console.log('try');
		parametri=decodeURIComponent(e.response);
		console.log('decoded: ' + parametri);
		parametri = JSON.parse(parametri);
		//parametri ='{"1":0,"2":1,"3":1,"4":0,"5":0,"6":7,"7":51}';
		console.log('parametri: ' + parametri);
		
		try {
      Pebble.sendAppMessage(parametri);
      localStorage.setItem(setPebbleToken, parametri);
			console.log('setItem:'+ setPebbleToken + parametri);
    } catch(e) {
    }
  }
});

