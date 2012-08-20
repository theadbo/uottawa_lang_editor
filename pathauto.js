(function ($) {

Drupal.behaviors.pathFrenchFieldsetSummaries = {
  attach: function (context) {
    $('fieldset.path-form-french', context).drupalSetSummary(function (context) {
      var path = $('.form-item-path-french-alias-french input').val();
      var automatic = $('.form-item-path-french-pathauto-french input').attr('checked');

      if (automatic) {
        return Drupal.t('Automatic alias');
      }
      if (path) {
        return Drupal.t('Alias: @alias', { '@alias': path });
      }
      else {
        return Drupal.t('No alias');
      }
    });
  }
};

})(jQuery);
