
/**
 * @file
 * Attaches behaviors for the Path module.
 */

(function ($) {

Drupal.behaviors.pathFrenchFieldsetSummaries = {
  attach: function (context) {
    $('fieldset.path-form-french', context).drupalSetSummary(function (context) {
      var path = $('.form-item-path-french-alias-french input', context).val();

      return path ?
        Drupal.t('Alias: @alias', { '@alias': path }) :
        Drupal.t('No alias');
    });
  }
};

})(jQuery);
