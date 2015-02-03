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
chronoControllers.controller('FeaturesCtrl', ['$scope', function FeaturesCtrl($scope)
{

}]);
chronoControllers.controller('DownloadsCtrl', ['$scope', '$http', function DownloadsCtrl($scope, $http)
{
    $scope.downloads = null;
    $http.get("contents/downloads.json").success(function(result) {
        console.log(result);
        $scope.downloads = result;
    });
}]);

chronoControllers.controller('TodoCtrl', ['$scope', '$http', function TodoCtrl($scope, $http)
{
    $scope.todoList = null;
    $http.get("contents/todo.json").success(function(result) {
        $scope.todoList = result;
    });
}]);

chronoControllers.controller('FaqCtrl', ['$scope', '$http', function FaqCtrl($scope, $http)
{

}]);

chronoControllers.controller('DocumentationCtrl', ['$scope', function DocumentationCtrl($scope)
{

}]);

chronoControllers.controller('AboutCtrl', ['$scope', function AboutCtrl($scope)
{

}]);