/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */


(function($) {
  $(document).ready(function() {
    $("li:has(a[href*='/translate'])").remove();
  });
}(jQuery));