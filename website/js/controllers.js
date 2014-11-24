var chronoControllers = angular.module('chronoControllers', []);

//----------------------------------------------------------------------------------------------------
//		Navigation
//----------------------------------------------------------------------------------------------------
chronoControllers.controller('NavCtrl', ['$scope', '$location', function NavCtrl($scope, $location)
{
    $scope.menuClass = function(page) {
        var current = $location.path().substring(1);
        return page === current ? "active" : "";
    };
}]);


chronoControllers.controller('HomeCtrl', ['$scope', function HomeCtrl($scope)
{

}]);
chronoControllers.controller('DownloadsCtrl', ['$scope', function DownloadsCtrl($scope)
{

}]);

chronoControllers.controller('FaqCtrl', ['$scope', function FaqCtrl($scope)
{

}]);

chronoControllers.controller('DocumentationCtrl', ['$scope', function DocumentationCtrl($scope)
{

}]);

chronoControllers.controller('AboutCtrl', ['$scope', function AboutCtrl($scope)
{

}]);