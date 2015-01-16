if(typeof(base_dir) === 'undefined')
	var base_dir = '';

//----------------------------------------------------------------------------------------------------
//		App Declaration
//----------------------------------------------------------------------------------------------------
var chronoApp = angular.module('chronoApp', [
	'ngRoute',
    'chronoControllers'
]);

//----------------------------------------------------------------------------------------------------
//		App Config (config providers here)
//----------------------------------------------------------------------------------------------------
chronoApp.config(['$httpProvider', '$routeProvider', function($httpProvider, $routeProvider)
{
    $routeProvider.when('/home', {
        templateUrl: 'partials/home.html',
        controller: 'HomeCtrl'
    }).when('/features', {
        templateUrl: 'partials/features.html',
        controller: 'FeaturesCtrl'
    }).when('/downloads', {
        templateUrl: 'partials/downloads.html',
        controller: 'DownloadsCtrl'
    }).when('/faq', {
        templateUrl: 'partials/faq.html',
        controller: 'FaqCtrl'
    }).when('/todo', {
        templateUrl: 'partials/todo.html',
        controller: 'TodoCtrl'
    }).when('/documentation', {
        templateUrl: 'partials/documentation.html',
        controller: 'DocumentationCtrl'
    }).when('/about', {
        templateUrl: 'partials/about.html',
        controller: 'AboutCtrl'
    }).
    otherwise({
        redirectTo: '/home'
    });
	//------------------------------------------------
	//	Params as Object when sending request
	//------------------------------------------------
	$httpProvider.defaults.headers.post['Content-Type'] = 'application/x-www-form-urlencoded;charset=utf-8';
	$httpProvider.defaults.cache = false;
	$httpProvider.defaults.transformRequest = [function(data)
	{
		return angular.isObject(data) && String(data) !== '[object File]' ? $.param(data) : data;
	}];
}]);

